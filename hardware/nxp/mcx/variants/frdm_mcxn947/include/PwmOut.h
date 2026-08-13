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

#elif defined( CPU_MCXN947VDF )

/**
 * FRDM-MCXN947 PwmOut.
 *
 * | Logical pin | Physical pin | PWM0 submodule | Channel | PORT3 ALT |
 * |-------------|--------------|-----------------|---------|-----------|
 * | PWM_0       | P3_11        | sm3             | B       | Alt5      |
 * | PWM_1       | P3_10        | sm3             | A       | Alt4      |
 * | PWM_2       | P3_9         | sm2             | B       | Alt4      |
 * | PWM_3       | P3_8         | sm2             | A       | Alt5      |
 * | PWM_4       | P3_7         | sm1             | B       | Alt4      |
 * | PWM_5       | P3_6         | sm1             | A       | Alt4      |
 *
 * Same 6 physical pins (P3_6..P3_11) as A153's PWM0-PWM5, but named
 * PWM_0.."PWM_5" (not "PWM0".."PWM5") -- this chip's SDK already defines a
 * bare `PWM0` macro for the FlexPWM peripheral instance itself
 * ((PWM_Type*)PWM0_BASE), so reusing that name for the logical pin would
 * make every `PWM_Init(PWM0,...)` call below silently expand to a pin
 * number instead of the peripheral pointer. A153 didn't hit this because
 * its SDK instance macro is "FLEXPWM0", not "PWM0". Submodule numbers
 * also differ from A153 (sm1/2/3 here, not sm0/1/2 -- confirmed from
 * pin_mux.c's pin_signal strings, e.g. P3_6's alt-function list is
 * ".../CT4_MAT2/PWM0_A1/...", and the digit after A/B is the submodule
 * number per NXP's own naming). ALT mux values are NOT uniform across
 * these 6 pins (unlike A153) -- each pin's PWM0_Ax/Bx entry sits at a
 * different position in its own alt-function list depending on how many
 * other functions (e.g. an extra WUU0_INxx) precede it, so each pin needs
 * its own ALT value (derived by counting position in the pin_signal
 * string, the same method already verified against I3C1_SDA's Alt10).
 * The instance itself is `PWM0` (PWM_Type*) -- N947 has no "FLEXPWM0"
 * macro, just PWM0/PWM1.
 */

extern "C" {
#include "fsl_pwm.h"
#include "fsl_clock.h"
#include "fsl_port.h"
}

#include "obj.h"
#include "io.h"

class PwmOut : public Obj
{
public:
    explicit PwmOut( int pin );
    virtual ~PwmOut();

    void  period( float seconds );
    void  period_ms( int ms );
    void  period_us( int us );

    void  pulsewidth( float seconds );
    void  pulsewidth_ms( int ms );
    void  pulsewidth_us( int us );

    void  write( float duty );
    float read( void );

    PwmOut &operator=( float duty );
    operator float();

private:
    void resolve_pin( int pin );
    void apply( void );

    static void _acquire_module( void );
    static void _release_module( void );
    static uint8_t _instance_count;

    uint8_t  _submodule;   // 1=sm1, 2=sm2, 3=sm3
    uint8_t  _channel;     // 0=chA, 1=chB
    int      _pin;

    uint32_t _period_us;
    uint32_t _pulse_us;
};

#endif // defined( CPU_MCXA153VLH ) / defined( CPU_MCXN947VDF )

#endif // R01LIB_PWMOUT_H
