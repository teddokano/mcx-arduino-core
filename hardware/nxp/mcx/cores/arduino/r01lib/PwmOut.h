/**
 * @file    PwmOut.h
 * @brief   Mbed-compatible PwmOut class for r01lib (FlexPWM)
 *
 * PWM output on the pins FlexPWM reaches, with a Mbed-like API. Which
 * pins those are, and on which FlexPWM instance, submodule and channel,
 * is a per-chip table in PwmOut.cpp:
 *
 * | Board         | Pins                                   | FlexPWM        |
 * |---------------|----------------------------------------|----------------|
 * | FRDM-MCXA153  | PWM0-PWM5 (P3_11..P3_6)                | FlexPWM0       |
 * | FRDM-MCXA156  | PWM0-PWM5 (P3_11..P3_6)                | FlexPWM0       |
 * |               | D3, D5, D6, D9 (P3_12, P3_14, P3_16, P3_17) | FlexPWM1  |
 * | FRDM-MCXN947  | PWM0-PWM5 (P2_2..P2_7)                 | FlexPWM1       |
 * | FRDM-MCXN236  | PWM0-PWM5 (four of them are D3/D5/D6/D9) | FlexPWM1     |
 *
 * ### Example usage
 * @code
 * PwmOut pwm( PWM0 );
 * pwm.period_ms( 20 );
 * pwm.write( 0.5f );    // 50% duty
 * @endcode
 *
 * ### Design notes -- IMPORTANT
 * - **The period is shared per submodule.** Channels A and B of a
 *   submodule share one period register; only the duty is independent.
 *   period() on one PwmOut changes the period of the other channel of its
 *   submodule too, and that channel keeps its pulse width (clamped to the
 *   new period). A PwmOut made on a submodule whose other
 *   channel is already running takes over the running period instead of
 *   resetting it. This is a hardware constraint, documented here per Mbed
 *   HAL convention (same approach as shared-timer PWM pins on official
 *   Mbed targets).
 * - Each FlexPWM instance is taken out of reset by the first PwmOut on it,
 *   and each submodule is initialized (fault-disable map cleared) by the
 *   first PwmOut on that submodule.
 * - ~PwmOut() only sets the duty to 0; it does not revert the pin to GPIO
 *   (matches Mbed's pwmout_free() scope).
 * - Periods and pulse widths are kept in ticks of FlexPWM's clock (96MHz on
 *   FRDM-MCXA153/A156, 150MHz on FRDM-MCXN947/N236), before the prescaler,
 *   and set to the nearest tick: the duty has as many steps as the 16-bit
 *   counter counts in one period at any frequency.
 *
 * @author  Tedd OKANO
 * @copyright MIT License
 */

#ifndef R01LIB_PWMOUT_H
#define R01LIB_PWMOUT_H

#if defined( CPU_MCXA153VLH ) || defined( CPU_MCXA156VLL ) || defined( CPU_MCXN947VDF ) || defined( CPU_MCXN236VDF )

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
     * Resolves @p pin to a FlexPWM instance, submodule and channel, takes
     * the instance out of reset and initializes the submodule if nothing
     * has yet, and starts the channel at 0% duty. The period is 20ms, or
     * the period the submodule's other channel is already running at.
     * Calls `panic()` if @p pin is not a supported PWM pin.
     *
     * @param pin  Logical PWM-capable pin (`PWM0`..`PWM5`, and on
     *             FRDM-MCXA156 also `D3`, `D5`, `D6`, `D9`).
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
    /** @brief  Set the period from a frequency in Hz, to the nearest tick
     *  of the PWM's clock (analogWriteFrequency()). */
    void  frequency( uint32_t hz );

    /** @brief  Set pulse width in seconds (clamped to period). */
    void  pulsewidth( float seconds );
    /** @brief  Set pulse width in milliseconds. */
    void  pulsewidth_ms( int ms );
    /** @brief  Set pulse width in microseconds. */
    void  pulsewidth_us( int us );

    /** @brief  Set duty cycle, 0.0 - 1.0. */
    void  write( float duty );
    /** @brief  Set the duty as @p num / @p den of the period, to the nearest
     *  tick, with no float: analogWrite()'s value over its largest value. */
    void  write_ratio( uint32_t num, uint32_t den );
    /** @brief  Get current duty cycle, 0.0 - 1.0. */
    float read( void );

    /** @brief  Mbed-style assignment, equivalent to write(). */
    PwmOut &operator=( float duty );
    /** @brief  Mbed-style implicit conversion, equivalent to read(). */
    operator float();

    /** @brief  Put the pin back on FlexPWM after something else took it.
     *
     *  pinMode() switches a pin to GPIO without knowing about any PwmOut
     *  on it. This muxes the pin to FlexPWM again and records this PwmOut
     *  as its owner; the channel's period and duty are as they were.
     */
    void  claim_pin( void );

    /** @brief  Stop recording this PwmOut as the pin's owner.
     *
     *  For a caller about to switch the pin to another function. The
     *  channel itself keeps running, so claim_pin() can take the pin back.
     */
    void  release_pin( void );

    /** @brief  Is this pin one FlexPWM can actually drive?
     *
     *  Asks the same table the constructor resolves against, without
     *  constructing anything -- the constructor panic()s on a pin it
     *  cannot drive, so callers that want to handle that case themselves
     *  (analogWrite() falls back to digitalWrite, as AVR's core does)
     *  have to ask first.
     *
     * @param pin raw r01lib pin number, *not* an Arduino-renumbered one
     * @return true if FlexPWM reaches this pin
     */
    static bool is_pwm_pin( int pin );

    /** @brief  Is the other channel of this pin's submodule running?
     *
     *  If so, a PwmOut made on @p pin takes over that channel's period,
     *  and setting a period on it changes the other channel's too.
     *
     * @param pin raw r01lib pin number, *not* an Arduino-renumbered one
     * @return true if a PwmOut already exists on this pin's submodule
     */
    static bool shares_running_period( int pin );

private:
    void apply( void );  // push the submodule's period and both channels' pulses to hardware
    void set_period( uint32_t ticks );  // in source-clock ticks, clamped to what the counter can count
    void set_pulse( uint32_t ticks );   // in source-clock ticks, clamped to the period

    uint8_t  _module;      // index into PwmOut.cpp's FlexPWM instance table
    uint8_t  _submodule;   // 0=sm0, 1=sm1, 2=sm2
    uint8_t  _channel;     // 0=chA, 1=chB
    uint8_t  _alt;         // PORT mux ALT for the pin
    int      _pin;
};

#elif defined( CPU_MCXC444VLH )

extern "C" {
#include "fsl_tpm.h"
#include "fsl_clock.h"
#include "fsl_port.h"
}

#include "obj.h"
#include "io.h"

/**
 * @brief Mbed-compatible PWM output class, on FRDM-MCXC444's TPMs.
 *
 *  D3 is TPM1_CH0, D10 is TPM0_CH4, and D5, D6 and D9 are all TPM0_CH2
 *  (Zephyr's MCXC444VLH-pinctrl.h). A TPM has one period for all its
 *  channels, so every pin on TPM0 shares it. D5, D6 and D9 share one
 *  channel too: the PwmOut written last drives it, and the pin of the one
 *  it takes the channel from goes back to GPIO, driven LOW.
 *
 *  Periods and pulse widths are kept in ticks of the TPMs' 48MHz clock,
 *  before the prescaler, and set to the nearest tick.
 */
class PwmOut : public Obj
{
public:
    /** @param pin D3, D5, D6, D9 or D10 (raw r01lib pin values) */
    explicit PwmOut( int pin );
    /** Sets the pulse to 0 if this PwmOut still drives its channel */
    virtual ~PwmOut();

    /** @brief  Set period in seconds. Shared by every pin on the same TPM. */
    void  period( float seconds );
    /** @brief  Set period in milliseconds. */
    void  period_ms( int ms );
    /** @brief  Set period in microseconds. */
    void  period_us( int us );
    /** @brief  Set the period from a frequency in Hz, to the nearest tick
     *  of the PWM's clock (analogWriteFrequency()). */
    void  frequency( uint32_t hz );

    /** @brief  Set pulse width in seconds (clamped to period). */
    void  pulsewidth( float seconds );
    /** @brief  Set pulse width in milliseconds. */
    void  pulsewidth_ms( int ms );
    /** @brief  Set pulse width in microseconds. */
    void  pulsewidth_us( int us );

    /** @brief  Set duty cycle, 0.0 - 1.0. */
    void  write( float duty );
    /** @brief  Set the duty as @p num / @p den of the period, to the nearest
     *  tick, with no float: analogWrite()'s value over its largest value. */
    void  write_ratio( uint32_t num, uint32_t den );
    /** @brief  Get current duty cycle, 0.0 - 1.0. */
    float read( void );

    /** @brief  Mbed-style assignment, equivalent to write(). */
    PwmOut &operator=( float duty );
    /** @brief  Mbed-style implicit conversion, equivalent to read(). */
    operator float();

    /** @brief  Put the pin back on its TPM channel after something else took it
     *  (and take the channel from another pin sharing it) */
    void  claim_pin( void );
    /** @brief  Stop recording this PwmOut as the pin's owner, for a caller
     *  about to switch the pin to another function */
    void  release_pin( void );

    /** @param pin raw r01lib pin number @return true if a TPM channel reaches it */
    static bool is_pwm_pin( int pin );
    /** @param pin raw r01lib pin number @return true if this pin's TPM is already running */
    static bool shares_running_period( int pin );

private:
    void apply( void );  // push the TPM's period and its channels' pulses to hardware
    void take_channel( void );
    void set_period( uint32_t ticks );  // in source-clock ticks, clamped to what the counter can count
    void set_pulse( uint32_t ticks );   // in source-clock ticks, clamped to the period

    uint8_t  _tpm;        // 0 = TPM0, 1 = TPM1
    uint8_t  _channel;    // TPMn_CHx
    uint8_t  _alt;        // PORT mux ALT for the pin
    bool     _holds_pin;  // the pin is muxed to the TPM for this PwmOut
    uint32_t _pulse_ticks;  // in ticks of the TPM's source clock, before the prescaler
    int      _pin;
};

/** Writes the CnV values that had to wait for a TPM's period to end.
 *  Called from SysTick_Handler() every millisecond, with micros() */
void pwm_tpm_service( uint32_t now );

#else
#error "PwmOut.h: no PWM outputs for this chip"
#endif

#endif // R01LIB_PWMOUT_H
