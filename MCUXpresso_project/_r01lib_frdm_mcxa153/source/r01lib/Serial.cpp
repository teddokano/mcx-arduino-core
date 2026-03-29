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
    port_mux_t        mux;
    reset_ip_name_t   rst;
    IRQn_Type         irqn;
    clock_attach_id_t clk_attach;
    clock_div_name_t  clk_div;
};

static const lpuart_pin_map_t s_pinMap[] = {
    //  TX       RX       base     inst  mux             rst                       irqn           clk_attach              clk_div
    { P0_3,  P0_2,  LPUART0, 0U, kPORT_MuxAlt2, kLPUART0_RST_SHIFT_RSTn, LPUART0_IRQn, kFRO12M_to_LPUART0, kCLOCK_DivLPUART0 }, // USBTX/USBRX
    { MB_TX, MB_RX, LPUART2, 2U, kPORT_MuxAlt2, kLPUART2_RST_SHIFT_RSTn, LPUART2_IRQn, kFRO12M_to_LPUART2, kCLOCK_DivLPUART2 }, // MikroBus
    { D1,    D0,    LPUART2, 2U, kPORT_MuxAlt3, kLPUART2_RST_SHIFT_RSTn, LPUART2_IRQn, kFRO12M_to_LPUART2, kCLOCK_DivLPUART2 }, // Arduino D0/D1
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
            _mux        = s_pinMap[i].mux;
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
    port_mux_t        mux;
    reset_ip_name_t   rst;
    IRQn_Type         irqn;
    clock_attach_id_t clk_attach;
    clock_div_name_t  clk_div;
};

static const lpuart_pin_map_t s_pinMap[] = {
    //  TX      RX      base     inst  mux             rst                        irqn          clk_attach           clk_div
    { USBTX, USBRX, LPUART0, 0U, kPORT_MuxAlt2, kLPUART0_RST_SHIFT_RSTn, LPUART0_IRQn, kFRO12M_to_LPUART0, kCLOCK_DivLPUART0 }, // P0_3/P0_2
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
            _mux        = s_pinMap[i].mux;
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
    port_mux_t        mux;
    reset_ip_name_t   rst;
    IRQn_Type         irqn;
    clock_attach_id_t clk_attach;
};

static const lpuart_pin_map_t s_pinMap[] = {
    //  USBTX=P1_9(TX) / USBRX=P1_8(RX) -> FC4 LPUART4, Alt2
    { USBTX, USBRX, LPUART4, 4U, kPORT_MuxAlt2, kFC4_RST_SHIFT_RSTn, LP_FLEXCOMM4_IRQn, kFRO12M_to_FLEXCOMM4 },
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
            _mux        = s_pinMap[i].mux;
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
}

struct lpuart_pin_map_t {
    int               tx_pin, rx_pin;
    LPUART_Type      *base;
    uint32_t          instance;
    port_mux_t        mux;
    reset_ip_name_t   rst;
    IRQn_Type         irqn;
    clock_attach_id_t clk_attach;
};

static const lpuart_pin_map_t s_pinMap[] = {
    //  USBTX=P1_9(TX) / USBRX=P1_8(RX) -> FC4 LPUART4, Alt2
    { USBTX, USBRX, LPUART4, 4U, kPORT_MuxAlt2, kFC4_RST_SHIFT_RSTn, LP_FLEXCOMM4_IRQn, kFRO12M_to_FLEXCOMM4 },
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
            _mux        = s_pinMap[i].mux;
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
    port_mux_t      mux;
    IRQn_Type       irqn;
    clock_ip_name_t clk_gate;
};

// C444 pin-to-LPUART mapping (verified against RM and pin_mux.c):
//   USBTX = D1 = PTA2  -> LPUART0_TX  Alt2
//   USBRX = D0 = PTA1  -> LPUART0_RX  Alt2
//   MB_TX = PTE0       -> LPUART1_TX  Alt3
//   MB_RX = PTE1       -> LPUART1_RX  Alt3
static const lpuart_pin_map_t s_pinMap[] = {
    { D1,    D0,    LPUART0, 0U, kPORT_MuxAlt2, LPUART0_IRQn, kCLOCK_Lpuart0 }, // PTA2/PTA1 (= USBTX/USBRX)
    { MB_TX, MB_RX, LPUART1, 1U, kPORT_MuxAlt3, LPUART1_IRQn, kCLOCK_Lpuart1 }, // PTE0/PTE1
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
            _mux       = s_pinMap[i].mux;
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
      _instance( 0U ), _mux( kPORT_MuxAlt2 ), _irqn( NotAvail_IRQn ),
      _tx_pin( tx ), _rx_pin( rx ),
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

    // Set pin mux for TX and RX before LPUART_Init
    {
        DigitalInOut tx_io( (uint8_t)tx );
        DigitalInOut rx_io( (uint8_t)rx );
        tx_io.pin_mux( (int)_mux );
        rx_io.pin_mux( (int)_mux );
    }

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

Serial::~Serial()
{
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
        uint8_t  byte = LPUART_ReadByte( _base );
        uint16_t next = (uint16_t)(( _rx_head + 1U ) & ( RX_RING_BUF_SIZE - 1U ));

        if ( next != _rx_tail )   // drop silently if ring buffer full
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
    LPUART_DisableInterrupts( _base,
        kLPUART_RxDataRegFullInterruptEnable |
        kLPUART_TxDataRegEmptyInterruptEnable );

    _config.baudRate_Bps = (uint32_t)baudrate;
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

bool Serial::writable( void )
{
    uint16_t next = (uint16_t)(( _tx_head + 1U ) & ( TX_RING_BUF_SIZE - 1U ));
    return ( next != _tx_tail );
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
