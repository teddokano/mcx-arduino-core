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
 * | Logical pin | Physical pin | FlexPWM1 submodule | Channel | PORT2 ALT |
 * |-------------|--------------|---------------------|---------|-----------|
 * | PWM0        | P2_3         | sm2                 | B       | Alt5      |
 * | PWM1        | P2_2         | sm2                 | A       | Alt5      |
 * | PWM2        | P2_5         | sm1                 | B       | Alt5      |
 * | PWM3        | P2_4         | sm1                 | A       | Alt5      |
 * | PWM4        | P2_7         | sm0                 | B       | Alt5      |
 * | PWM5        | P2_6         | sm0                 | A       | Alt5      |
 *
 * NOTE (corrected twice after real-hardware bring-up):
 *
 * 1. An earlier version of this driver reused A153's physical pin numbers
 *    (P3_6..P3_11) verbatim. On N947 those pins are bare test points in
 *    pin_mux.c's schematic labels (TP8/TP12-18/TP31), not routed to any
 *    populated header -- analogWrite() compiled, linked, and even produced
 *    a clean PWM waveform on the die pad, but nobody could reach it from
 *    outside the board. Caught via logic analyzer finding nothing on the
 *    documented pin; the user traced the real, header-accessible
 *    PWM-capable pins in the schematic (FRDM-MCXN947SH.pdf, "Arduino
 *    Shield Compatible Headers" sheet, page 12): P2_2..P2_7, wired to
 *    FlexPWM1 (not FlexPWM0). The PWM0..PWM5 assignment above matches
 *    the "PWM0".."PWM5" silkscreen labels printed directly on that header
 *    in the schematic, which is why it isn't in physical pin order.
 *
 * 2. The first fix for (1) derived each pin's ALT value by counting
 *    position in pin_mux.c's pin_signal string -- the same method already
 *    verified against I3C1_SDA's Alt10 and (coincidentally) correct for
 *    P2_4..P2_7 here. It gave Alt6 for P2_2 and Alt4 for P2_3, both wrong:
 *    P2_2's pin_signal list has an extra "CLKOUT" entry ahead of PWM1_A2
 *    that doesn't actually consume a mux slot, throwing the count off by
 *    one, and P2_3 was wrong for a similar reason. Both silently produced
 *    zero PWM output on all six probed pins (not just the two wrong ones)
 *    on real hardware -- reported by the user via logic analyzer, no
 *    signal anywhere. Re-derived from Zephyr's silicon-accurate pinctrl
 *    header instead (MCXN947VDF-pinctrl.h, `N9X_MUX(port,pin,mux)`
 *    macros), which shows ALT is uniformly 5 for all six PWM1_Ax/Bx pins
 *    here. Lesson: position-counting in pin_mux.c's comment text is a
 *    useful fallback but not reliable on its own -- cross-check against an
 *    authoritative pinctrl source when one is available, and always
 *    confirm the final result on real hardware.
 *
 * This chip's SDK defines bare `PWM0`/`PWM1` macros for the FlexPWM
 * peripheral instances themselves ((PWM_Type*)PWM0_BASE / PWM1_BASE) --
 * A153 doesn't hit this (its SDK instance macro is "FLEXPWM0", not
 * "PWM0"/"PWM1"), but here io.h has to explicitly reclaim both names
 * (#undef, same technique as source/r01device/led/LEDDriver.h's identical
 * collision) before redefining them as logical pin names. PwmOut.cpp
 * captures the SDK's original PWM1 meaning into its own alias before
 * including this header, so its own `PWM_Init(...)` calls etc. keep
 * referring to the actual peripheral rather than a pin number.
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

    uint8_t  _submodule;   // 0=sm0, 1=sm1, 2=sm2
    uint8_t  _channel;     // 0=chA, 1=chB
    int      _pin;

    uint32_t _period_us;
    uint32_t _pulse_us;
};

#endif // defined( CPU_MCXA153VLH ) / defined( CPU_MCXN947VDF )

#endif // R01LIB_PWMOUT_H
