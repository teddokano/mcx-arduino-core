/**
 * @file    Serial.h
 * @brief   Mbed-compatible Serial class for r01lib
 *
 * Provides interrupt-driven, ring-buffered UART communication with a
 * Mbed-like API across multiple NXP MCX Freedom boards.
 *
 * ### Supported targets and pin pairs
 *
 * | Board        | CPU macro          | Supported pin pairs                   |
 * |--------------|--------------------|---------------------------------------|
 * | FRDM-MCXC444 | CPU_MCXC444VLH     | USBTX/USBRX (= D1/D0), MB_TX/MB_RX   |
 * | FRDM-MCXA153 | CPU_MCXA153VLH     | USBTX/USBRX, D1/D0, MB_TX/MB_RX      |
 * | FRDM-MCXA156 | CPU_MCXA156VLL     | USBTX/USBRX                           |
 * | FRDM-MCXN236 | CPU_MCXN236VDF     | USBTX/USBRX                           |
 * | FRDM-MCXN947 | CPU_MCXN947VDF     | USBTX/USBRX                           |
 *
 * ### Example usage
 * @code
 * Serial uart( USBTX, USBRX, 115200 );
 * uart.printf( "Hello\r\n" );
 *
 * // Interrupt-driven receive
 * void on_rx( void ) {
 *     int c = uart.getc();
 * }
 * uart.attach( on_rx, Serial::RxIrq );
 * @endcode
 *
 * ### Internal architecture
 * Both TX and RX use software ring buffers driven by the LPUART interrupt,
 * without the SDK Transfer API.
 *
 * - **RX path**: `kLPUART_RxDataRegFullInterruptEnable` fires →
 *   ISR reads one byte → pushes into `_rx_buf` → calls `_rx_callback`.
 * - **TX path**: `putc` / `write` / `printf` push bytes into `_tx_buf`
 *   and enable `kLPUART_TxDataRegEmptyInterruptEnable` →
 *   ISR pops one byte → writes to hardware.
 *   When `_tx_buf` is empty the ISR disables the TX interrupt and calls
 *   `_tx_callback` (if attached).
 *
 * @author  Tedd OKANO
 * @license MIT
 */

#ifndef R01LIB_SERIAL_H
#define R01LIB_SERIAL_H

extern "C" {
#include "fsl_lpuart.h"
#include "fsl_clock.h"
#include "fsl_port.h"
}

#include <stdarg.h>
#include "obj.h"
#include "io.h"

/** Pointer type for Serial interrupt callbacks. */
typedef void (*func_ptr)( void );

/**
 * @brief Mbed-compatible UART class backed by interrupt-driven ring buffers.
 *
 * Instantiate with a TX pin, RX pin, and optional baud rate.  The constructor
 * resolves the hardware peripheral, configures clocks, mux, and enables the
 * LPUART.  The destructor cleanly disables interrupts and de-initialises the
 * peripheral.
 */
class Serial : public Obj
{
public:
    /**
     * @brief Interrupt source selector passed to attach().
     */
    enum IrqType {
        RxIrq = 0,  /**< Receive data register full. */
        TxIrq = 1,  /**< Transmit data register empty (TX buffer drained). */
    };

    /** Size of the software receive ring buffer (bytes). */
    static constexpr size_t RX_RING_BUF_SIZE = 64U;
    /** Size of the software transmit ring buffer (bytes). */
    static constexpr size_t TX_RING_BUF_SIZE = 256U;

    /**
     * @brief  Construct and initialise the Serial port.
     *
     * Resolves the @p tx / @p rx pin pair to the appropriate LPUART
     * peripheral, configures the port mux, sets up clocks, releases reset,
     * and calls `LPUART_Init`.  Calls `panic()` if the pin combination is not
     * supported on the current target.
     *
     * @param tx    Transmit pin (e.g. `USBTX`, `MB_TX`).
     * @param rx    Receive pin  (e.g. `USBRX`, `MB_RX`).
     * @param baud  Baud rate in bps (default 115200).
     */
    Serial( int tx, int rx, int baud = 115200 );

    /**
     * @brief  Destroy the Serial port and release hardware resources.
     *
     * Disables RX/TX interrupts, disables the NVIC IRQ, calls
     * `LPUART_Deinit`, and unregisters the instance from the IRQ dispatch
     * table.
     */
    virtual ~Serial();

    /**
     * @brief  Change the baud rate at runtime.
     *
     * Temporarily disables interrupts, re-initialises the peripheral, then
     * restores interrupt enables according to the current callback state.
     *
     * @param baudrate  New baud rate in bps.
     */
    void     baud( int baudrate );

    /**
     * @brief  Write a single character to the TX ring buffer.
     *
     * Blocks (spin-waits) if the TX ring buffer is full.
     *
     * @param c  Character to transmit (only the low 8 bits are used).
     * @return   The character written, cast to `int`.
     */
    int      putc( int c );

    /**
     * @brief  Read one character.
     *
     * If an RX callback has been attached, reads from the RX ring buffer and
     * returns -1 if no data is available.  If no callback is attached,
     * blocks until a byte is received via `LPUART_ReadBlocking`.
     *
     * @return  Received byte as `int` (0–255), or -1 if the ring buffer is
     *          empty and interrupt-driven mode is active.
     */
    int      getc( void );

    /**
     * @brief  Formatted print to the serial port.
     *
     * Formats into a 256-byte stack buffer then enqueues each byte into the
     * TX ring buffer.
     *
     * @param fmt  `printf`-style format string.
     * @param ...  Variadic arguments matching @p fmt.
     * @return     Number of characters written (same semantics as `vsnprintf`).
     */
    int      printf( const char *fmt, ... );

    /**
     * @brief  Query whether at least one byte is available to read.
     *
     * @return `true` if data is available; `false` otherwise.
     */
    bool     readable( void );

    /**
     * @brief  Query whether there is space in the TX ring buffer.
     *
     * @return `true` if at least one byte can be enqueued without blocking.
     */
    bool     writable( void );

    /**
     * @brief  Write a byte array to the TX ring buffer.
     *
     * Enqueues all @p length bytes.  Blocks on each byte if the TX ring
     * buffer is momentarily full.
     *
     * @param data    Pointer to the data to transmit.
     * @param length  Number of bytes to transmit.
     * @return        `kStatus_Success`.
     */
    status_t write( const uint8_t *data, size_t length );

    /**
     * @brief  Blocking read of exactly @p length bytes.
     *
     * Delegates to `LPUART_ReadBlocking`; does not use the RX ring buffer.
     *
     * @param data    Destination buffer.
     * @param length  Number of bytes to read.
     * @return        SDK status code from `LPUART_ReadBlocking`.
     */
    status_t read( uint8_t *data, size_t length );

    /**
     * @brief  Register a callback for RX or TX interrupts.
     *
     * Attaching an `RxIrq` callback enables the LPUART receive interrupt and
     * switches `getc` / `readable` to ring-buffer mode.
     * Attaching a `TxIrq` callback stores a function that is called once the
     * TX ring buffer has been fully drained.
     *
     * Pass `nullptr` to remove a previously registered callback.
     *
     * @param callback  Function to call on the selected interrupt event.
     * @param type      `RxIrq` (default) or `TxIrq`.
     */
    void     attach( func_ptr callback, IrqType type = RxIrq );

    /**
     * @brief  Internal: called from the global LPUART IRQ handler.
     *
     * Not intended for direct use by application code.
     */
    void _irq_handler( void );

private:
    // ---- target-specific helpers (implemented per #ifdef in Serial.cpp) ----
    void     resolve_pins( int tx, int rx );
    void     _setup_clock( void );
    uint32_t _get_clk_freq( void );
    void     _release_reset( void );
    void     _register_instance( void );
    void     _unregister_instance( void );

    // ---- common helpers ----
    void tx_enqueue( uint8_t b );
    void update_irq_enables( void );

    // ---- hardware ----
    LPUART_Type    *_base;
    lpuart_config_t _config;
    uint32_t        _clk_freq;
    uint32_t        _instance;
    port_mux_t      _mux;
    IRQn_Type       _irqn;
    int             _tx_pin;
    int             _rx_pin;

    // ---- target-specific clock / reset fields ----
#if defined( CPU_MCXA153VLH ) || defined( CPU_MCXA156VLL )
    reset_ip_name_t   _rst;
    clock_attach_id_t _clk_attach;
    clock_div_name_t  _clk_div;
#elif defined( CPU_MCXN236VDF ) || defined( CPU_MCXN947VDF )
    reset_ip_name_t   _rst;
    clock_attach_id_t _clk_attach;
#elif defined( CPU_MCXC444VLH )
    clock_ip_name_t   _clk_gate;
#endif

    // ---- RX ring buffer (ISR writes, getc reads) ----
    volatile uint8_t  _rx_buf[ RX_RING_BUF_SIZE ];
    volatile uint16_t _rx_head;
    volatile uint16_t _rx_tail;

    // ---- TX ring buffer (putc/write writes, ISR reads) ----
    volatile uint8_t  _tx_buf[ TX_RING_BUF_SIZE ];
    volatile uint16_t _tx_head;
    volatile uint16_t _tx_tail;

    // ---- user callbacks ----
    func_ptr _rx_callback;
    func_ptr _tx_callback;
};

#endif // R01LIB_SERIAL_H
