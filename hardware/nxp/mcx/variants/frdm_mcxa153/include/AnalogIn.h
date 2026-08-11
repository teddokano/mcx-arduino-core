/**
 * @file    AnalogIn.h
 * @brief   Mbed-compatible AnalogIn class for r01lib (FRDM-MCXA153 first)
 *
 * Provides single-shot (poll-per-read) ADC access with a Mbed-like API.
 * Modeled on the pin/channel knowledge verified in the IchigoJam_Z
 * (Zephyr) port for FRDM-MCXA153:
 *
 * | ANA()  | Logical pin | Physical pin | LPADC0 channel_id | input_positive |
 * |--------|-------------|--------------|--------------------|----------------|
 * | ANA(1) | A0          | P1_10        | 0                  | 8              |
 * | ANA(2) | A1          | P1_12        | 1                  | 10             |
 * | ANA(3) | A2          | P1_13        | 2                  | 11             |
 * | ANA(4) | A3          | P2_0         | 3                  | 0              |
 *
 * Reference: VDDA (ADC_REF_EXTERNAL0-equivalent), 12-bit conversion.
 *
 * ### Example usage
 * @code
 * AnalogIn ain( A0 );
 * float    v   = ain.read();      // 0.0 - 1.0
 * uint16_t v16 = ain.read_u16();  // 0 - 0xFFFF
 * @endcode
 *
 * ### Design notes (see project discussion)
 * - Single LPADC0 peripheral is shared across all AnalogIn instances.
 *   Peripheral init (`LPADC_Init` + calibration) happens once, guarded by
 *   a reference count — same pattern as Serial::_register_instance().
 * - Each read() performs a full single-shot conversion (channel select +
 *   software trigger + poll for done + result read). No persistent
 *   hardware state is held between reads, so instances never conflict.
 * - read() blocks (polls) until conversion completes — do not call from
 *   ISR context.
 *
 * @author  Tedd OKANO
 * @license MIT
 */

#ifndef R01LIB_ANALOGIN_H
#define R01LIB_ANALOGIN_H

#if defined( CPU_MCXA153VLH )

extern "C" {
#include "fsl_lpadc.h"
#include "fsl_clock.h"
#include "fsl_port.h"
}

#include "obj.h"
#include "io.h"

/**
 * @brief Mbed-compatible single-shot ADC input class.
 */
class AnalogIn : public Obj
{
public:
    /**
     * @brief  Construct and configure an analog input on the given pin.
     *
     * Resolves @p pin to an LPADC0 channel_id / input_positive pair,
     * performs peripheral init (first instance only) and per-channel
     * `LPADC_SetConvCommandConfig`. Calls `panic()` if @p pin is not a
     * supported analog pin on the current target.
     *
     * @param pin  Logical analog pin (e.g. `A0`..`A3`).
     */
    explicit AnalogIn( int pin );

    /**
     * @brief  Destroy the AnalogIn. Does not stop LPADC0 (may be shared).
     */
    virtual ~AnalogIn();

    /**
     * @brief  Perform a single-shot conversion and return normalized value.
     * @return Value in range 0.0 (0V) - 1.0 (VDDA).
     */
    float read( void );

    /**
     * @brief  Perform a single-shot conversion and return 16-bit value.
     * @return Value in range 0 - 0xFFFF (12-bit ADC result left-scaled).
     */
    uint16_t read_u16( void );

    /** @brief  Mbed-style implicit conversion, equivalent to read(). */
    operator float();

private:
    void     resolve_pin( int pin );
    uint16_t sample_raw( void );   // single-shot conversion, 12-bit raw

    // ---- shared peripheral state (reference-counted) ----
    static void _acquire_peripheral( void );
    static void _release_peripheral( void );
    static uint8_t _instance_count;
    static bool    _calibrated;

    // ---- per-instance channel mapping ----
    uint8_t _channel_id;      // LPADC command/slot index
    uint8_t _input_positive;  // hardware ADC0_An number
    int     _pin;
};

#endif // defined( CPU_MCXA153VLH )

#endif // R01LIB_ANALOGIN_H
