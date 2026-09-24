/*
 *  Serial.cpp - Mbed-compatible Serial class for r01lib
 *
 *  Supports:
 *    FRDM-MCXC444  (CPU_MCXC444VLH)   LPUART0/1
 *    FRDM-MCXA153  (CPU_MCXA153VLH)   LPUART0/1/2
 *    FRDM-MCXA156  (CPU_MCXA156VLL)   LPUART0
 *    FRDM-MCXN236  (CPU_MCXN236VDF)   LP_FLEXCOMM4 (LPUART4)
 *    FRDM-MCXN947  (CPU_MCXN947VDF)   LP_FLEXCOMM4 (LPUART4)
 *
 *  Design note:
 *    Both TX and RX use software ring buffers driven directly by the LPUART
 *    interrupt, without the SDK Transfer API.
 *
 *    RX path:
 *      kLPUART_RxDataRegFullInterruptEnable fires ->
 *        ISR reads one byte -> pushes into _rx_buf -> calls _rx_callback.
 *
 *    TX path:
 *      putc/write/printf push bytes into _tx_buf and enable
 *      kLPUART_TxDataRegEmptyInterruptEnable ->
 *        ISR pops one byte -> writes to hardware.
 *        When _tx_buf is empty the ISR disables TX interrupt and calls
 *        _tx_callback (if attached).
 *
 *  @author  Based on r01lib pattern by Tedd OKANO
 *  Released under the MIT License
 */

extern "C" {
#include "fsl_lpuart.h"
#include "fsl_clock.h"
#include "fsl_port.h"
#include "fsl_common.h"
#if defined( CPU_MCXN236VDF ) || defined( CPU_MCXN947VDF )
#  include "fsl_lpflexcomm.h"
#  include "fsl_reset.h"
#elif defined( CPU_MCXA153VLH ) || defined( CPU_MCXA156VLL )
#  include "fsl_reset.h"
#endif
}

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "io.h"
#include "mcu.h"
#include "Serial.h"
#include "pin_registry.h"

// ===========================================================================
//  MCX A153  (LPUART0/1/2, PORT mux, CLOCK_AttachClk / CLOCK_SetClockDiv)
// ===========================================================================
#if defined( CPU_MCXA153VLH )

static Serial *s_instances[ 3 ] = { nullptr, nullptr, nullptr };

extern "C"
{
    void LPUART0_IRQHandler( void ) { if ( s_instances[0] ) s_instances[0]->_irq_handler(); SDK_ISR_EXIT_BARRIER; }
    void LPUART1_IRQHandler( void ) { if ( s_instances[1] ) s_instances[1]->_irq_handler(); SDK_ISR_EXIT_BARRIER; }
    void LPUART2_IRQHandler( void ) { if ( s_instances[2] ) s_instances[2]->_irq_handler(); SDK_ISR_EXIT_BARRIER; }
}

struct lpuart_pin_map_t {
    int               tx_pin, rx_pin;
    LPUART_Type      *base;
    uint32_t          instance;
    port_mux_t        tx_mux, rx_mux;	// TX and RX aren't always the same ALT
    reset_ip_name_t   rst;
    IRQn_Type         irqn;
    clock_attach_id_t clk_attach;
    clock_div_name_t  clk_div;
};

static const lpuart_pin_map_t s_pinMap[] = {
    //  TX       RX       base     inst  tx_mux         rx_mux         rst                       irqn           clk_attach              clk_div
    { P0_3,  P0_2,  LPUART0, 0U, kPORT_MuxAlt2, kPORT_MuxAlt2, kLPUART0_RST_SHIFT_RSTn, LPUART0_IRQn, kFRO12M_to_LPUART0, kCLOCK_DivLPUART0 }, // USBTX/USBRX
    { MB_TX, MB_RX, LPUART2, 2U, kPORT_MuxAlt2, kPORT_MuxAlt2, kLPUART2_RST_SHIFT_RSTn, LPUART2_IRQn, kFRO12M_to_LPUART2, kCLOCK_DivLPUART2 }, // MikroBus
    // Arduino D0/D1: both P1_5(D1)/LPUART2_TXD and P1_4(D0)/LPUART2_RXD are
    // Alt3 (confirmed against Zephyr's silicon-verified pinctrl tables,
    // MCXA344VLH-pinctrl.h: LPUART2_RXD_P1_4/LPUART2_TXD_P1_5 both mux=3).
    // The pin_mux.c pin_signal comment's function ORDER does not encode the
    // ALT number -- that assumption was wrong, don't rely on it again.
    { D1,    D0,    LPUART2, 2U, kPORT_MuxAlt3, kPORT_MuxAlt3, kLPUART2_RST_SHIFT_RSTn, LPUART2_IRQn, kFRO12M_to_LPUART2, kCLOCK_DivLPUART2 }, // Arduino D0/D1
};

void Serial::resolve_pins( int tx, int rx )
{
    _base = nullptr;
    for ( size_t i = 0; i < sizeof(s_pinMap)/sizeof(s_pinMap[0]); i++ )
    {
        if ( s_pinMap[i].tx_pin == tx && s_pinMap[i].rx_pin == rx )
        {
            _base       = s_pinMap[i].base;
            _instance   = s_pinMap[i].instance;
            _tx_mux     = s_pinMap[i].tx_mux;
            _rx_mux     = s_pinMap[i].rx_mux;
            _irqn       = s_pinMap[i].irqn;
            _rst        = s_pinMap[i].rst;
            _clk_attach = s_pinMap[i].clk_attach;
            _clk_div    = s_pinMap[i].clk_div;
            return;
        }
    }
}

void     Serial::_setup_clock( void )    { CLOCK_SetClockDiv( _clk_div, 1U ); CLOCK_AttachClk( _clk_attach ); }
uint32_t Serial::_get_clk_freq( void )   { return CLOCK_GetLpuartClkFreq( _instance ); }
void     Serial::_release_reset( void )  { RESET_ReleasePeripheralReset( _rst ); }
void     Serial::_register_instance( void )   { s_instances[ _instance ] = this; }
void     Serial::_unregister_instance( void ) { s_instances[ _instance ] = nullptr; }


// ===========================================================================
//  MCX A156  (LPUART0, PORT mux, CLOCK_AttachClk / CLOCK_SetClockDiv)
// ===========================================================================
#elif defined( CPU_MCXA156VLL )

static Serial *s_instances[ 1 ] = { nullptr };

extern "C"
{
    void LPUART0_IRQHandler( void ) { if ( s_instances[0] ) s_instances[0]->_irq_handler(); SDK_ISR_EXIT_BARRIER; }
}

struct lpuart_pin_map_t {
    int               tx_pin, rx_pin;
    LPUART_Type      *base;
    uint32_t          instance;
    port_mux_t        tx_mux, rx_mux;
    reset_ip_name_t   rst;
    IRQn_Type         irqn;
    clock_attach_id_t clk_attach;
    clock_div_name_t  clk_div;
};

static const lpuart_pin_map_t s_pinMap[] = {
    //  TX      RX      base     inst  tx_mux         rx_mux         rst                        irqn          clk_attach           clk_div
    { USBTX, USBRX, LPUART0, 0U, kPORT_MuxAlt2, kPORT_MuxAlt2, kLPUART0_RST_SHIFT_RSTn, LPUART0_IRQn, kFRO12M_to_LPUART0, kCLOCK_DivLPUART0 }, // P0_3/P0_2
};

void Serial::resolve_pins( int tx, int rx )
{
    _base = nullptr;
    for ( size_t i = 0; i < sizeof(s_pinMap)/sizeof(s_pinMap[0]); i++ )
    {
        if ( s_pinMap[i].tx_pin == tx && s_pinMap[i].rx_pin == rx )
        {
            _base       = s_pinMap[i].base;
            _instance   = s_pinMap[i].instance;
            _tx_mux     = s_pinMap[i].tx_mux;
            _rx_mux     = s_pinMap[i].rx_mux;
            _irqn       = s_pinMap[i].irqn;
            _rst        = s_pinMap[i].rst;
            _clk_attach = s_pinMap[i].clk_attach;
            _clk_div    = s_pinMap[i].clk_div;
            return;
        }
    }
}

void     Serial::_setup_clock( void )    { CLOCK_SetClockDiv( _clk_div, 1U ); CLOCK_AttachClk( _clk_attach ); }
uint32_t Serial::_get_clk_freq( void )   { return CLOCK_GetLpuartClkFreq( _instance ); }
void     Serial::_release_reset( void )  { RESET_ReleasePeripheralReset( _rst ); }
void     Serial::_register_instance( void )   { s_instances[ _instance ] = this; }
void     Serial::_unregister_instance( void ) { s_instances[ _instance ] = nullptr; }

// ===========================================================================
//  MCX N236  (LP_FLEXCOMM4, CLOCK_AttachClk, CLOCK_GetLPFlexCommClkFreq)
// ===========================================================================
#elif defined( CPU_MCXN236VDF )

static Serial *s_instances[ 8 ] = {};

extern "C"
{
    void LP_FLEXCOMM4_IRQHandler( void ) { if ( s_instances[4] ) s_instances[4]->_irq_handler(); SDK_ISR_EXIT_BARRIER; }
}

struct lpuart_pin_map_t {
    int               tx_pin, rx_pin;
    LPUART_Type      *base;
    uint32_t          instance;
    port_mux_t        tx_mux, rx_mux;
    reset_ip_name_t   rst;
    IRQn_Type         irqn;
    clock_attach_id_t clk_attach;
};

static const lpuart_pin_map_t s_pinMap[] = {
    //  USBTX=P1_9(TX) / USBRX=P1_8(RX) -> FC4 LPUART4, Alt2
    { USBTX, USBRX, LPUART4, 4U, kPORT_MuxAlt2, kPORT_MuxAlt2, kFC4_RST_SHIFT_RSTn, LP_FLEXCOMM4_IRQn, kFRO12M_to_FLEXCOMM4 },
};

void Serial::resolve_pins( int tx, int rx )
{
    _base = nullptr;
    for ( size_t i = 0; i < sizeof(s_pinMap)/sizeof(s_pinMap[0]); i++ )
    {
        if ( s_pinMap[i].tx_pin == tx && s_pinMap[i].rx_pin == rx )
        {
            _base       = s_pinMap[i].base;
            _instance   = s_pinMap[i].instance;
            _tx_mux     = s_pinMap[i].tx_mux;
            _rx_mux     = s_pinMap[i].rx_mux;
            _irqn       = s_pinMap[i].irqn;
            _rst        = s_pinMap[i].rst;
            _clk_attach = s_pinMap[i].clk_attach;
            return;
        }
    }
}

void     Serial::_setup_clock( void )    { CLOCK_AttachClk( _clk_attach ); }
uint32_t Serial::_get_clk_freq( void )   { return CLOCK_GetLPFlexCommClkFreq( _instance ); }
void     Serial::_release_reset( void )  { RESET_PeripheralReset( _rst ); RESET_ReleasePeripheralReset( _rst ); LP_FLEXCOMM_Init( _instance, LP_FLEXCOMM_PERIPH_LPUART ); }
void     Serial::_register_instance( void )   { s_instances[ _instance ] = this; }
void     Serial::_unregister_instance( void ) { s_instances[ _instance ] = nullptr; }


// ===========================================================================
//  MCX N947  (LP_FLEXCOMM4, CLOCK_AttachClk, CLOCK_GetLPFlexCommClkFreq)
// ===========================================================================
#elif defined( CPU_MCXN947VDF )

static Serial *s_instances[ 10 ] = {};

extern "C"
{
    void LP_FLEXCOMM4_IRQHandler( void ) { if ( s_instances[4] ) s_instances[4]->_irq_handler(); SDK_ISR_EXIT_BARRIER; }
    void LP_FLEXCOMM5_IRQHandler( void ) { if ( s_instances[5] ) s_instances[5]->_irq_handler(); SDK_ISR_EXIT_BARRIER; }
}

struct lpuart_pin_map_t {
    int               tx_pin, rx_pin;
    LPUART_Type      *base;
    uint32_t          instance;
    port_mux_t        tx_mux, rx_mux;
    reset_ip_name_t   rst;
    IRQn_Type         irqn;
    clock_attach_id_t clk_attach;
};

static const lpuart_pin_map_t s_pinMap[] = {
    //  USBTX=P1_9(TX) / USBRX=P1_8(RX) -> FC4 LPUART4, Alt2
    { USBTX, USBRX, LPUART4, 4U, kPORT_MuxAlt2, kPORT_MuxAlt2, kFC4_RST_SHIFT_RSTn, LP_FLEXCOMM4_IRQn, kFRO12M_to_FLEXCOMM4 },
    //  MB_TX=P1_17(TX) / MB_RX=P1_16(RX) -> FC5 LPUART5, Alt2 (MikroBus, Serial1)
    { MB_TX, MB_RX, LPUART5, 5U, kPORT_MuxAlt2, kPORT_MuxAlt2, kFC5_RST_SHIFT_RSTn, LP_FLEXCOMM5_IRQn, kFRO12M_to_FLEXCOMM5 },
};

void Serial::resolve_pins( int tx, int rx )
{
    _base = nullptr;
    for ( size_t i = 0; i < sizeof(s_pinMap)/sizeof(s_pinMap[0]); i++ )
    {
        if ( s_pinMap[i].tx_pin == tx && s_pinMap[i].rx_pin == rx )
        {
            _base       = s_pinMap[i].base;
            _instance   = s_pinMap[i].instance;
            _tx_mux     = s_pinMap[i].tx_mux;
            _rx_mux     = s_pinMap[i].rx_mux;
            _irqn       = s_pinMap[i].irqn;
            _rst        = s_pinMap[i].rst;
            _clk_attach = s_pinMap[i].clk_attach;
            return;
        }
    }
}

void     Serial::_setup_clock( void )    { CLOCK_AttachClk( _clk_attach ); }
uint32_t Serial::_get_clk_freq( void )   { return CLOCK_GetLPFlexCommClkFreq( _instance ); }
void     Serial::_release_reset( void )  { RESET_PeripheralReset( _rst ); RESET_ReleasePeripheralReset( _rst ); LP_FLEXCOMM_Init( _instance, LP_FLEXCOMM_PERIPH_LPUART ); }
void     Serial::_register_instance( void )   { s_instances[ _instance ] = this; }
void     Serial::_unregister_instance( void ) { s_instances[ _instance ] = nullptr; }


// ===========================================================================
//  MCX C444  (SIM SCGC5 clock gate, CLOCK_SetLpuartXClock, no RESET module)
// ===========================================================================
#elif defined( CPU_MCXC444VLH )

static Serial *s_instances[ 2 ] = { nullptr, nullptr };

extern "C"
{
    void LPUART0_IRQHandler( void ) { if ( s_instances[0] ) s_instances[0]->_irq_handler(); SDK_ISR_EXIT_BARRIER; }
    void LPUART1_IRQHandler( void ) { if ( s_instances[1] ) s_instances[1]->_irq_handler(); SDK_ISR_EXIT_BARRIER; }
}

struct lpuart_pin_map_t {
    int             tx_pin, rx_pin;
    LPUART_Type    *base;
    uint32_t        instance;
    port_mux_t      tx_mux, rx_mux;
    IRQn_Type       irqn;
    clock_ip_name_t clk_gate;
};

// C444 pin-to-LPUART mapping (verified against RM and pin_mux.c):
//   USBTX = D1 = PTA2  -> LPUART0_TX  Alt2
//   USBRX = D0 = PTA1  -> LPUART0_RX  Alt2
//   MB_TX = PTE0       -> LPUART1_TX  Alt3
//   MB_RX = PTE1       -> LPUART1_RX  Alt3
static const lpuart_pin_map_t s_pinMap[] = {
    { D1,    D0,    LPUART0, 0U, kPORT_MuxAlt2, kPORT_MuxAlt2, LPUART0_IRQn, kCLOCK_Lpuart0 }, // PTA2/PTA1 (= USBTX/USBRX)
    { MB_TX, MB_RX, LPUART1, 1U, kPORT_MuxAlt3, kPORT_MuxAlt3, LPUART1_IRQn, kCLOCK_Lpuart1 }, // PTE0/PTE1
};

void Serial::resolve_pins( int tx, int rx )
{
    _base = nullptr;
    for ( size_t i = 0; i < sizeof(s_pinMap)/sizeof(s_pinMap[0]); i++ )
    {
        if ( s_pinMap[i].tx_pin == tx && s_pinMap[i].rx_pin == rx )
        {
            _base      = s_pinMap[i].base;
            _instance  = s_pinMap[i].instance;
            _tx_mux    = s_pinMap[i].tx_mux;
            _rx_mux    = s_pinMap[i].rx_mux;
            _irqn      = s_pinMap[i].irqn;
            _clk_gate  = s_pinMap[i].clk_gate;
            return;
        }
    }
}

void Serial::_setup_clock( void )
{
    CLOCK_EnableClock( _clk_gate );
    if ( _instance == 0U )
        CLOCK_SetLpuart0Clock( 1U );  // 1 = IRC48M (48 MHz)
    else
        CLOCK_SetLpuart1Clock( 1U );
}

uint32_t Serial::_get_clk_freq( void )  { return 48000000UL; }  // IRC48M
void     Serial::_release_reset( void ) { /* C444 has no RESET module */ }
void     Serial::_register_instance( void )   { s_instances[ _instance ] = this; }
void     Serial::_unregister_instance( void ) { s_instances[ _instance ] = nullptr; }

#else
#  error "Serial.cpp: unsupported target. Define one of: CPU_MCXC444VLH, CPU_MCXA153VLH, CPU_MCXA156VLL, CPU_MCXN236VDF, CPU_MCXN947VDF."
#endif  // target selection


// ===========================================================================
//  Common implementation  (all targets)
// ===========================================================================

Serial::Serial( int tx, int rx, int baud )
    : Obj( true ),
      _base( nullptr ), _config{}, _clk_freq( 0U ),
      _instance( 0U ), _tx_mux( kPORT_MuxAlt2 ), _rx_mux( kPORT_MuxAlt2 ), _irqn( NotAvail_IRQn ),
      _tx_pin( tx ), _rx_pin( rx ), _rx_data_mask( 0xFF ), _pins_muxed( false ),
      _rx_head( 0 ), _rx_tail( 0 ),
      _tx_head( 0 ), _tx_tail( 0 ),
      _rx_callback( nullptr ), _tx_callback( nullptr )
{
    resolve_pins( tx, rx );

    if ( !_base )
    {
        panic( "Serial: unsupported TX/RX pin combination" );
        return;
    }

    _register_instance();

    /*
     *  Note what is deliberately NOT done here: claiming the TX/RX pins.
     *  apply_pin_mux() does that, and begin() is what calls it.
     *
     *  The global Serial/Serial1 instances are constructed during static
     *  initialization, before main(), whether or not the sketch ever
     *  begin()s them. Muxing the pins here meant Serial1 seized its pins
     *  unconditionally at that point -- and on FRDM-MCXN947 Serial1 sits
     *  on MB_TX/MB_RX, the very same physical pins as I3C_SCL/I3C_SDA.
     *  A sketch with its own global I3C object was therefore in an
     *  unwinnable race: static init order across translation units is
     *  unspecified, so whichever constructor ran last owned the pins. When
     *  Serial1 lost that race nothing happened; when it won, the I3C
     *  peripheral was left disconnected from the bus, SDA/SCL idled high
     *  with no START ever generated, and every transfer returned
     *  kStatus_I3C_Nak. Wire1 escaped this only because TwoWire::begin()
     *  constructs its I3C lazily, long after static init.
     *
     *  Deferring to begin() also just matches how Arduino is supposed to
     *  behave -- a port you never begin() has no business holding pins.
     */

    // Clock must be set up before reset release and LPUART_Init
    _setup_clock();
    _release_reset();

    LPUART_GetDefaultConfig( &_config );
    _config.baudRate_Bps = (uint32_t)baud;
    _config.enableTx     = true;
    _config.enableRx     = true;

    _clk_freq = _get_clk_freq();
    LPUART_Init( _base, &_config, _clk_freq );
}

void Serial::apply_pin_mux( void )
{
    if ( !_base )
        return;

    DigitalInOut tx_io( (uint8_t)_tx_pin );
    DigitalInOut rx_io( (uint8_t)_rx_pin );

    tx_io.pin_mux( (int)_tx_mux );
    rx_io.pin_mux( (int)_rx_mux );

    // RX pins not otherwise pre-configured at boot (pin_mux.c) reset
    // with the digital input buffer disabled -- MUX alone isn't enough,
    // the peripheral never sees the incoming signal without this.
    rx_io.input_buffer( true );

    uint8_t pins[ 2 ] = { (uint8_t)_tx_pin, (uint8_t)_rx_pin };
    pin_registry_note( this, "Serial", pins, 2, (uint8_t)_tx_mux );

    _pins_muxed = true;
}

Serial::~Serial()
{
    pin_registry_forget( this );

    if ( !_base )
        return;

    LPUART_DisableInterrupts( _base,
        kLPUART_RxDataRegFullInterruptEnable |
        kLPUART_TxDataRegEmptyInterruptEnable );
    DisableIRQ( _irqn );

    LPUART_Deinit( _base );
    _unregister_instance();
}

// ---------------------------------------------------------------------------
//  Private helpers
// ---------------------------------------------------------------------------

void Serial::tx_enqueue( uint8_t b )
{
    uint16_t next;
    do {
        next = (uint16_t)(( _tx_head + 1U ) & ( TX_RING_BUF_SIZE - 1U ));
    } while ( next == _tx_tail );   // spin if TX buffer full

    _tx_buf[ _tx_head ] = b;
    _tx_head = next;

    EnableIRQ( _irqn );
    LPUART_EnableInterrupts( _base, kLPUART_TxDataRegEmptyInterruptEnable );
}

void Serial::update_irq_enables( void )
{
    if ( _rx_callback )
    {
        EnableIRQ( _irqn );
        LPUART_EnableInterrupts( _base, kLPUART_RxDataRegFullInterruptEnable );
    }
    else
    {
        LPUART_DisableInterrupts( _base, kLPUART_RxDataRegFullInterruptEnable );
        if ( _tx_head == _tx_tail )
            DisableIRQ( _irqn );
    }
}

// ---------------------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------------------

void Serial::attach( func_ptr callback, IrqType type )
{
    if ( type == RxIrq )
    {
        _rx_callback = callback;
        update_irq_enables();
    }
    else
    {
        _tx_callback = callback;
    }
}

void Serial::_irq_handler( void )
{
    uint32_t flags = LPUART_GetStatusFlags( _base );

    // ---- RX: one byte received ----
    if ( flags & kLPUART_RxDataRegFullFlag )
    {
        // DATA read whole rather than through LPUART_ReadByte(), for its
        // per-byte parity-error bit.
        uint32_t data = _base->DATA;
        uint8_t  byte = (uint8_t)( data & _rx_data_mask );
        uint16_t next = (uint16_t)(( _rx_head + 1U ) & ( RX_RING_BUF_SIZE - 1U ));

        if ( flags & kLPUART_ParityErrorFlag )
            LPUART_ClearStatusFlags( _base, kLPUART_ParityErrorFlag );

        // Drop silently if the ring buffer is full, and drop a byte that
        // failed the parity check, as AVR's core does
        if ( next != _rx_tail && !( data & LPUART_DATA_PARITYE_MASK ) )
        {
            _rx_buf[ _rx_head ] = byte;
            _rx_head = next;
        }

        if ( _rx_callback )
            _rx_callback();
    }

    // ---- TX: register empty — send next byte from ring buffer ----
    if ( flags & kLPUART_TxDataRegEmptyFlag )
    {
        if ( _tx_head != _tx_tail )
        {
            LPUART_WriteByte( _base, _tx_buf[ _tx_tail ] );
            _tx_tail = (uint16_t)(( _tx_tail + 1U ) & ( TX_RING_BUF_SIZE - 1U ));
        }
        else
        {
            // TX buffer drained: disable interrupt, fire callback
            LPUART_DisableInterrupts( _base, kLPUART_TxDataRegEmptyInterruptEnable );
            if ( _tx_callback )
                _tx_callback();
        }
    }

    // ---- Overrun: clear flag ----
    if ( flags & kLPUART_RxOverrunFlag )
        LPUART_ClearStatusFlags( _base, kLPUART_RxOverrunFlag );
}

void Serial::baud( int baudrate )
{
    _config.baudRate_Bps = (uint32_t)baudrate;
    reinit();
}

void Serial::format( int bits, Parity parity, int stop_bits )
{
    if ( ( bits != 7 && bits != 8 ) ||
         ( parity != None && parity != Odd && parity != Even ) ||
         ( stop_bits != 1 && stop_bits != 2 ) )
    {
        panic( "Serial: unsupported format (7 or 8 data bits, no forced parity, 1 or 2 stop bits)" );
        return;
    }

    _config.dataBitsCount = ( bits == 7 ) ? kLPUART_SevenDataBits : kLPUART_EightDataBits;
    _config.parityMode    = ( parity == Odd )  ? kLPUART_ParityOdd
                          : ( parity == Even ) ? kLPUART_ParityEven
                          :                      kLPUART_ParityDisabled;
    _config.stopBitCount  = ( stop_bits == 2 ) ? kLPUART_TwoStopBit : kLPUART_OneStopBit;
    _rx_data_mask         = ( bits == 7 ) ? 0x7F : 0xFF;

    reinit();
}

void Serial::end( void )
{
    if ( !_base )
        return;

    flush();

    // Receiver and its interrupt off first, so nothing lands in the ring
    // buffer behind the reset below. baud() turns both back on.
    LPUART_DisableInterrupts( _base, kLPUART_RxDataRegFullInterruptEnable );
    _base->CTRL &= ~LPUART_CTRL_RE_MASK;
    while ( LPUART_GetStatusFlags( _base ) & kLPUART_RxDataRegFullFlag )
        (void)_base->DATA;
    _rx_tail = _rx_head;

    // Only pins this port actually took. On FRDM-MCXN947, Serial1 shares
    // its pins with Wire1, so ending a Serial1 that was never begun must
    // not pull them out from under Wire1.
    if ( !_pins_muxed )
        return;

    DigitalInOut tx_io( (uint8_t)_tx_pin );
    DigitalInOut rx_io( (uint8_t)_rx_pin );

    tx_io.pin_mux( 0 );
    rx_io.pin_mux( 0 );

    pin_registry_forget( this );
    _pins_muxed = false;
}

void Serial::reinit( void )
{
    LPUART_DisableInterrupts( _base,
        kLPUART_RxDataRegFullInterruptEnable |
        kLPUART_TxDataRegEmptyInterruptEnable );

    LPUART_Deinit( _base );
    LPUART_Init( _base, &_config, _clk_freq );

    update_irq_enables();

    if ( _tx_head != _tx_tail )
        LPUART_EnableInterrupts( _base, kLPUART_TxDataRegEmptyInterruptEnable );
}

int Serial::putc( int c )
{
    tx_enqueue( (uint8_t)c );
    return c;
}

int Serial::getc( void )
{
    if ( _rx_callback )
    {
        if ( _rx_head == _rx_tail )
            return -1;

        uint8_t b = _rx_buf[ _rx_tail ];
        _rx_tail  = (uint16_t)(( _rx_tail + 1U ) & ( RX_RING_BUF_SIZE - 1U ));
        return (int)b;
    }
    else
    {
        uint8_t b;
        LPUART_ReadBlocking( _base, &b, 1 );
        return (int)b;
    }
}

int Serial::peek( void )
{
    if ( _rx_callback )
    {
        if ( _rx_head == _rx_tail )
            return -1;

        return (int)_rx_buf[ _rx_tail ];
    }

    return -1;
}

int Serial::printf( const char *fmt, ... )
{
    char    buf[ 256 ];
    va_list ap;
    va_start( ap, fmt );
    int n = vsnprintf( buf, sizeof(buf), fmt, ap );
    va_end( ap );

    for ( int i = 0; i < n; i++ )
        tx_enqueue( (uint8_t)buf[ i ] );

    return n;
}

bool Serial::readable( void )
{
    if ( _rx_callback )
        return ( _rx_head != _rx_tail );

    return ( LPUART_GetStatusFlags( _base ) & kLPUART_RxDataRegFullFlag ) != 0U;
}

size_t Serial::available( void )
{
    if ( _rx_callback )
        return (size_t)(( _rx_head - _rx_tail ) & ( RX_RING_BUF_SIZE - 1U ));

    return readable() ? 1U : 0U;
}

bool Serial::writable( void )
{
    uint16_t next = (uint16_t)(( _tx_head + 1U ) & ( TX_RING_BUF_SIZE - 1U ));
    return ( next != _tx_tail );
}

size_t Serial::availableForWrite( void )
{
    size_t used = (size_t)(( _tx_head - _tx_tail ) & ( TX_RING_BUF_SIZE - 1U ));
    return (size_t)( TX_RING_BUF_SIZE - 1U - used );
}

void Serial::flush( void )
{
    while ( _tx_head != _tx_tail )
        ;

    while ( !( LPUART_GetStatusFlags( _base ) & kLPUART_TransmissionCompleteFlag ) )
        ;
}

status_t Serial::write( const uint8_t *data, size_t length )
{
    for ( size_t i = 0; i < length; i++ )
        tx_enqueue( data[ i ] );

    return kStatus_Success;
}

status_t Serial::read( uint8_t *data, size_t length )
{
    return LPUART_ReadBlocking( _base, data, length );
}

// ---------------------------------------------------------------------------
//  panic() output
// ---------------------------------------------------------------------------

void Serial::panic_write( const char *s )
{
    static Serial *made_here = nullptr;
    Serial        *port      = nullptr;

    for ( size_t i = 0; i < sizeof(s_pinMap)/sizeof(s_pinMap[0]); i++ )
        if ( s_pinMap[i].tx_pin == USBTX && s_pinMap[i].rx_pin == USBRX )
            port = s_instances[ s_pinMap[i].instance ];

    if ( !port )
    {
        if ( !made_here )
            made_here = new Serial( USBTX, USBRX );
        port = made_here;
    }

    port->write_polled( s );
}

void Serial::write_polled( const char *s )
{
    if ( !_base )
        return;

    DisableIRQ( _irqn );
    LPUART_DisableInterrupts( _base, kLPUART_TxDataRegEmptyInterruptEnable );

    // What the sketch printed just before the panic is still in the ring
    // buffer, and it's usually what explains the panic
    while ( _tx_tail != _tx_head )
    {
        uint8_t b = _tx_buf[ _tx_tail ];
        LPUART_WriteBlocking( _base, &b, 1 );
        _tx_tail = (uint16_t)(( _tx_tail + 1U ) & ( TX_RING_BUF_SIZE - 1U ));
    }

    if ( !_pins_muxed )
        apply_pin_mux();
    _base->CTRL |= LPUART_CTRL_TE_MASK;

    LPUART_WriteBlocking( _base, (const uint8_t *)s, strlen( s ) );
}
