/**
 * @file    PwmOut.h
 * @brief   Mbed-compatible PwmOut class for r01lib (FRDM-MCXA153 first)
 *
 * Provides FlexPWM0-backed PWM output with a Mbed-like API. Modeled on
 * the submodule/channel mapping verified in the IchigoJam_Z (Zephyr)
 * port for FRDM-MCXA153:
 *
 * | Logical pin | Physical pin | FlexPWM0 submodule | Channel |
 * |-------------|--------------|---------------------|---------|
 * | PWM0        | P3_11        | sm2                 | B       |
 * | PWM1        | P3_10        | sm2                 | A       |
 * | PWM2        | P3_9         | sm1                 | B       |
 * | PWM3        | P3_8         | sm1                 | A       |
 * | PWM4        | P3_7         | sm0                 | B       |
 * | PWM5        | P3_6         | sm0                 | A       |
 *
 * PWM0..PWM5 (defined in io.h) run in reverse physical-pin order to match
 * the on-board connector orientation. These are dedicated PWM pins on
 * FRDM-MCXA153 — no runtime pinmux switching between GPIO/PWM is needed
 * (unlike RP2040).
 *
 * ### Example usage
 * @code
 * PwmOut pwm( PWM0 );   // maps to a submodule/channel pair above
 * pwm.period_ms( 20 );
 * pwm.write( 0.5f );    // 50% duty
 * @endcode
 *
 * ### Design notes (see project discussion) — IMPORTANT
 * - **period() is shared per submodule.** Channels A/B on the same sm
 *   (e.g. port 1 & 2) share one period register; only duty is
 *   independent. Calling period() on one instance changes the period
 *   seen by the other channel sharing its submodule. This is a hardware
 *   constraint, not a bug — documented here per Mbed HAL convention
 *   (same approach as shared-timer PWM pins on official Mbed targets).
 * - FlexPWM0 fault-disable is configured once per module (first
 *   instance to touch FlexPWM0), same reference-counted pattern as
 *   Serial's clock/reset setup.
 * - ~PwmOut() only sets duty to 0; it does not revert the pin to GPIO
 *   (matches Mbed's pwmout_free() scope).
 *
 * @author  Tedd OKANO
 * @license MIT
 */

#ifndef R01LIB_PWMOUT_H
#define R01LIB_PWMOUT_H

#if defined( CPU_MCXA153VLH )

extern "C" {
#include "fsl_pwm.h"
#include "fsl_clock.h"
#include "fsl_port.h"
}

#include "obj.h"
#include "io.h"

/**
 * @brief Mbed-compatible PWM output class.
 */
class PwmOut : public Obj
{
public:
    /**
     * @brief  Construct and configure a PWM output on the given pin.
     *
     * Resolves @p pin to a FlexPWM0 submodule/channel pair, performs
     * module-level init (first instance only: clock, fault-disable),
     * and sets up the submodule/channel with a default 20ms period,
     * 0% duty. Calls `panic()` if @p pin is not a supported PWM pin.
     *
     * @param pin  Logical PWM-capable pin (e.g. `D3`..`D8` mapped to
     *             ports 1-6 above).
     */
    explicit PwmOut( int pin );

    /**
     * @brief  Destroy the PwmOut. Sets duty to 0 (does not free the pin).
     */
    virtual ~PwmOut();

    /** @brief  Set period in seconds. Shared with the paired channel. */
    void  period( float seconds );
    /** @brief  Set period in milliseconds. */
    void  period_ms( int ms );
    /** @brief  Set period in microseconds. */
    void  period_us( int us );

    /** @brief  Set pulse width in seconds (clamped to period). */
    void  pulsewidth( float seconds );
    /** @brief  Set pulse width in milliseconds. */
    void  pulsewidth_ms( int ms );
    /** @brief  Set pulse width in microseconds. */
    void  pulsewidth_us( int us );

    /** @brief  Set duty cycle, 0.0 - 1.0. */
    void  write( float duty );
    /** @brief  Get current duty cycle, 0.0 - 1.0. */
    float read( void );

    /** @brief  Mbed-style assignment, equivalent to write(). */
    PwmOut &operator=( float duty );
    /** @brief  Mbed-style implicit conversion, equivalent to read(). */
    operator float();

private:
    void resolve_pin( int pin );
    void apply( void );  // push _period_us / _pulse_us to hardware

    // ---- shared module state (reference-counted) ----
    static void _acquire_module( void );
    static void _release_module( void );
    static uint8_t _instance_count;

    // ---- per-instance submodule/channel mapping ----
    uint8_t  _submodule;   // 0=sm0, 1=sm1, 2=sm2
    uint8_t  _channel;     // 0=chA, 1=chB
    int      _pin;

    uint32_t _period_us;
    uint32_t _pulse_us;
};

#endif // defined( CPU_MCXA153VLH )

#endif // R01LIB_PWMOUT_H
