/** FRDM-MCXN947 Ethernet bring-up, below MCXEthernet: the ENET driver and
 *  the LAN8741A PHY, with no lwIP.
 *
 *  Run it when MCXEthernet doesn't work, to see whether the hardware path
 *  does. It uses the ENET and PHY drivers MCXEthernet carries (its
 *  mcx_port/ folder), but none of MCXEthernet itself: Ethernet.begin() is
 *  never called, so lwIP and its 1 ms interrupt stay off.
 *
 *  1. MDIO: reads the PHY's ID (needs no cable)
 *  2. waits up to 10 s for the link (needs the RJ45, J16, on a network)
 *  3. sends a DHCP DISCOVER as a raw frame once a second, as a DHCP client
 *     does, and waits up to 10 s for an OFFER (needs a DHCP server on that
 *     network: a router, or a Mac sharing its connection to its wired port;
 *     a Mac didn't answer the first DISCOVER)
 *
 *  Prints each check, then ALL OK or the number of FAILs. If 1 fails, the
 *  PHY doesn't answer on MDIO (P1_20/P1_21); if 2 fails with 1 OK, check
 *  the cable and the other end; if 3 fails with 2 OK, the frames don't make
 *  it out or back (the RMII pins or the 50MHz reference on P1_4), or the
 *  network has no DHCP server.
 */

#include <MCXEthernet.h>  // puts the library's driver files on the include path
#include <mcx_port/fsl_enet.h>
#include <mcx_port/fsl_phylan8741.h>

const uint8_t PHY_ADDRESS = 0;
const int RXBD_NUM = 4;
const int TXBD_NUM = 4;
const int BUF_SIZE = ENET_FRAME_MAX_FRAMELEN;

enet_rx_bd_struct_t rxDesc[RXBD_NUM] __attribute__((aligned(ENET_BUFF_ALIGNMENT)));
enet_tx_bd_struct_t txDesc[TXBD_NUM] __attribute__((aligned(ENET_BUFF_ALIGNMENT)));
uint8_t rxBuf[RXBD_NUM][BUF_SIZE] __attribute__((aligned(ENET_BUFF_ALIGNMENT)));
uint32_t rxBufAddr[RXBD_NUM];
uint8_t txBuf[TXBD_NUM][BUF_SIZE] __attribute__((aligned(ENET_BUFF_ALIGNMENT)));
int rxIdx = 0;
int txIdx = 0;

uint8_t mac[6] = { 0x02, 0x00, 0x00, 0x94, 0x70, 0x01 };  // locally administered
const uint32_t XID = 0x4D435831;                           // "MCX1"

phy_lan8741_resource_t phyResource;
phy_handle_t phyHandle;

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

status_t mdio_write(uint8_t phyAddr, uint8_t regAddr, uint16_t data) {
  return ENET_MDIOWrite(ENET0, phyAddr, regAddr, data);
}

status_t mdio_read(uint8_t phyAddr, uint8_t regAddr, uint16_t *pData) {
  return ENET_MDIORead(ENET0, phyAddr, regAddr, pData);
}

// RMII on PORT1, ALT9: TXCLK, TXEN, TXD0-1, RXDV, RXD0-1, MDC, MDIO
void enetPins() {
  static const uint8_t pins[] = { 4, 5, 6, 7, 13, 14, 15, 20, 21 };
  CLOCK_EnableClock(kCLOCK_Port1);
  for (unsigned i = 0; i < sizeof(pins); i++)
    PORT1->PCR[pins[i]] = PORT_PCR_MUX(9) | PORT_PCR_IBE(1);
}

uint16_t ipChecksum(const uint8_t *p, int len) {
  uint32_t sum = 0;
  for (int i = 0; i < len; i += 2)
    sum += (p[i] << 8) | p[i + 1];
  while (sum >> 16)
    sum = (sum & 0xFFFF) + (sum >> 16);
  return ~sum;
}

// Ethernet + IPv4 + UDP + BOOTP with a DHCP DISCOVER, broadcast
int buildDiscover(uint8_t *f) {
  memset(f, 0, BUF_SIZE);
  memset(f, 0xFF, 6);
  memcpy(f + 6, mac, 6);
  f[12] = 0x08;
  f[13] = 0x00;

  uint8_t *b = f + 42;  // BOOTP, after 14 + 20 + 8 bytes of headers
  b[0] = 1;             // request
  b[1] = 1;             // Ethernet
  b[2] = 6;
  b[4] = (uint8_t)(XID >> 24);
  b[5] = (uint8_t)(XID >> 16);
  b[6] = (uint8_t)(XID >> 8);
  b[7] = (uint8_t)XID;
  b[10] = 0x80;  // broadcast reply
  memcpy(b + 28, mac, 6);
  uint8_t *o = b + 236;
  const uint8_t options[] = { 0x63, 0x82, 0x53, 0x63, 53, 1, 1, 55, 3, 1, 3, 6, 255 };
  memcpy(o, options, sizeof(options));
  const int bootpLen = 300;

  uint8_t *u = f + 34;
  int udpLen = 8 + bootpLen;
  u[1] = 68;
  u[3] = 67;
  u[4] = udpLen >> 8;
  u[5] = udpLen;

  uint8_t *ip = f + 14;
  int ipLen = 20 + udpLen;
  ip[0] = 0x45;
  ip[2] = ipLen >> 8;
  ip[3] = ipLen;
  ip[8] = 64;
  ip[9] = 17;
  memset(ip + 16, 0xFF, 4);
  uint16_t cs = ipChecksum(ip, 20);
  ip[10] = cs >> 8;
  ip[11] = cs;
  return 14 + ipLen;
}

bool sendFrame(const uint8_t *data, int len) {
  if (ENET_IsTxDescriptorDmaOwn(&txDesc[txIdx]))
    return false;
  memcpy(txBuf[txIdx], data, len);
  ENET_SetupTxDescriptor(&txDesc[txIdx], txBuf[txIdx], len, NULL, 0, len, false, false, kENET_FirstLastFlag, 0);
  txIdx = (txIdx + 1) % TXBD_NUM;
  ENET_UpdateTxDescriptorTail(ENET0, 0, txIdx ? (uint32_t)&txDesc[txIdx] : (uint32_t)&txDesc[TXBD_NUM]);
  return true;
}

// the frame's length, 0 if none, -1 for a frame with an error
int receiveFrame(uint8_t **data) {
  uint32_t control = ENET_GetRxDescriptor(&rxDesc[rxIdx]);
  if (control & ENET_RXDESCRIP_RD_OWN_MASK)
    return 0;
  if ((control & ENET_RXDESCRIP_WR_LD_MASK) && !(control & ENET_RXDESCRIP_WR_ERRSUM_MASK)) {
    *data = rxBuf[rxIdx];
    return control & ENET_RXDESCRIP_WR_PACKETLEN_MASK;
  }
  return -1;
}

void releaseFrame() {
  ENET_UpdateRxDescriptor(&rxDesc[rxIdx], rxBuf[rxIdx], NULL, false, false);
  ENET_UpdateRxDescriptorTail(ENET0, 0, (uint32_t)&rxDesc[RXBD_NUM]);
  rxIdx = (rxIdx + 1) % RXBD_NUM;
}

bool isOffer(const uint8_t *f, int len, uint8_t *yiaddr) {
  if (len < 42 + 240 || f[12] != 0x08 || f[13] != 0x00 || f[23] != 17)
    return false;
  const uint8_t *u = f + 14 + (f[14] & 0x0F) * 4;
  if (u[2] != 0 || u[3] != 68)
    return false;
  const uint8_t *b = u + 8;
  uint32_t x = ((uint32_t)b[4] << 24) | ((uint32_t)b[5] << 16) | ((uint32_t)b[6] << 8) | b[7];
  if (b[0] != 2 || x != XID)
    return false;
  memcpy(yiaddr, b + 16, 4);
  for (const uint8_t *o = b + 240; o + 2 < f + len && *o != 255; o += (*o == 0) ? 1 : 2 + o[1])
    if (o[0] == 53 && o[2] == 2)
      return true;
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\r\n=== FRDM-MCXN947 Ethernet bring-up (ENET and PHY, no lwIP) ===");

  enetPins();
  CLOCK_AttachClk(kNONE_to_ENETRMII);
  CLOCK_EnableClock(kCLOCK_Enet);
  SYSCON0->PRESETCTRL2 = SYSCON_PRESETCTRL2_ENET_RST_MASK;
  SYSCON0->PRESETCTRL2 &= ~SYSCON_PRESETCTRL2_ENET_RST_MASK;
  ENET_SetSMI(ENET0, CLOCK_GetCoreSysClkFreq());

  // 1. MDIO
  uint16_t id1 = 0, id2 = 0;
  bool readOk = mdio_read(PHY_ADDRESS, PHY_ID1_REG, &id1) == kStatus_Success
                && mdio_read(PHY_ADDRESS, PHY_ID2_REG, &id2) == kStatus_Success;
  Serial.print("PHY ID 0x");
  Serial.print(id1, HEX);
  Serial.print(" 0x");
  Serial.print(id2, HEX);
  Serial.println(" (LAN8741A: 0x7 0xC12x)");
  check("PHY answers on MDIO with the LAN8741A's ID", readOk && id1 == 0x0007 && (id2 & 0xFFF0) == 0xC120);

  phyResource.read = mdio_read;
  phyResource.write = mdio_write;
  phy_config_t phyConfig = {};
  phyConfig.phyAddr = PHY_ADDRESS;
  phyConfig.ops = &phylan8741_ops;
  phyConfig.resource = &phyResource;
  phyConfig.autoNeg = true;
  bool phyOk = PHY_Init(&phyHandle, &phyConfig) == kStatus_Success;
  check("PHY_Init()", phyOk);

  // 2. link
  bool link = false, autonego = false;
  uint32_t start = millis();
  while (phyOk && millis() - start < 10000) {
    PHY_GetAutoNegotiationStatus(&phyHandle, &autonego);
    PHY_GetLinkStatus(&phyHandle, &link);
    if (link && autonego)
      break;
    delay(10);
  }
  check("link up within 10 s", link && autonego);

  if (link && autonego) {
    phy_speed_t speed;
    phy_duplex_t duplex;
    PHY_GetLinkSpeedDuplex(&phyHandle, &speed, &duplex);
    Serial.print("link after ");
    Serial.print(millis() - start);
    Serial.print(" ms, ");
    Serial.print(speed == kPHY_Speed100M ? "100M" : "10M");
    Serial.println(duplex == kPHY_FullDuplex ? " full duplex" : " half duplex");

    // 3. a DHCP DISCOVER out, an OFFER back
    for (int i = 0; i < RXBD_NUM; i++)
      rxBufAddr[i] = (uint32_t)rxBuf[i];
    enet_buffer_config_t bufferConfig = {
      RXBD_NUM, TXBD_NUM, &txDesc[0], &txDesc[0], NULL, &rxDesc[0], &rxDesc[RXBD_NUM], &rxBufAddr[0], BUF_SIZE,
    };
    enet_config_t config;
    ENET_GetDefaultConfig(&config);
    config.miiSpeed = (enet_mii_speed_t)speed;
    config.miiDuplex = (enet_mii_duplex_t)duplex;
    ENET_Init(ENET0, &config, mac, 50000000U);
    ENET_DescriptorInit(ENET0, &config, &bufferConfig);
    ENET_StartRxTx(ENET0, 1, 1);

    static uint8_t frame[BUF_SIZE];
    int frames = 0, errors = 0, sent = 0;
    bool queued = true, offered = false;
    uint8_t yiaddr[4];
    uint32_t lastSend = 0;
    start = millis();
    while (millis() - start < 10000 && !offered) {
      if (millis() - lastSend >= 1000) {
        queued = queued && sendFrame(frame, buildDiscover(frame));
        lastSend = millis();
        sent++;
      }
      uint8_t *data;
      int n = receiveFrame(&data);
      if (n == 0)
        continue;
      if (n < 0)
        errors++;
      else {
        frames++;
        offered = isOffer(data, n, yiaddr);
      }
      releaseFrame();
    }
    check("each DHCP DISCOVER queued", queued);
    Serial.print("DISCOVERs sent: ");
    Serial.print(sent);
    Serial.print(", frames received: ");
    Serial.print(frames);
    Serial.print(", with errors: ");
    Serial.println(errors);
    if (offered) {
      Serial.print("offered ");
      Serial.println(IPAddress(yiaddr));
    }
    check("DHCP OFFER within 10 s", offered);
  }

  if (failCount == 0)
    Serial.println("ALL OK");
  else {
    Serial.print(failCount);
    Serial.println(" FAILED");
  }
}

void loop() {
}
