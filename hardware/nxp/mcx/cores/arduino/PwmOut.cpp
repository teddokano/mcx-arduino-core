/**
 * @file    PwmOut.cpp
 * @brief   Mbed-compatible PwmOut implementation for r01lib (FRDM-MCXA153)
 *
 * Verified against the FRDM-MCXA153 SDK FlexPWM driver example
 * (examples/driver_examples/pwm) and MCXA153_features.h before writing
 * this file:
 *  - FSL_FEATURE_PWM_FAULT_CH_COUNT is 1 on this part, so only
 *    `kPWM_faultchannel_0` (a single DISMAP register per submodule) exists.
 *  - The SDK doc for `PWM_SetupFaultDisableMap()` states "a reset sets all
 *    bits in this field" — i.e. out of reset, *every* FAULTx input is
 *    mapped to disable each channel's output. Since these fault pins are
 *    not driven by anything on this design, the disable-map is explicitly
 *    cleared for both channels of a submodule the first time it is used,
 *    otherwise the PWM output may never toggle. This is the concern the
 *    project memo flagged as needing verification before implementation.
 *  - `PWM_SetupPwm()` only supports integer 0-100% duty. For the
 *    continuous-duty Mbed API this file uses the raw-tick API
 *    `PWM_UpdatePwmPeriodAndDutycycle()` (16-bit period/duty) for all
 *    period()/write() calls after the one-time `PWM_SetupPwm()` call that
 *    establishes OUTEN/polarity/fault-state for the channel.
 *
 * @author  Tedd OKANO
 * @license MIT
 */

#if defined( CPU_MCXA153VLH )

extern "C" {
#include "fsl_reset.h"
}

#include "PwmOut.h"
#include "mcu.h"

uint8_t PwmOut::_instance_count = 0;

namespace {

struct PwmPinDescriptor {
    int      pin;         // io.h logical pin (PWM0..PWM5)
    uint32_t port_pin;     // PORT3 pin number
    uint8_t  submodule;    // 0=sm0, 1=sm1, 2=sm2
    uint8_t  channel;      // 0=chA, 1=chB
};

const PwmPinDescriptor s_pins[] = {
    //  pin    pin#  sm  ch
    { PWM5,  6u, 0u, 0u },
    { PWM4,  7u, 0u, 1u },
    { PWM3,  8u, 1u, 0u },
    { PWM2,  9u, 1u, 1u },
    { PWM1, 10u, 2u, 0u },
    { PWM0, 11u, 2u, 1u },
};

const clock_ip_name_t s_sm_clock[ 3 ] = { kCLOCK_GatePWMSM0, kCLOCK_GatePWMSM1, kCLOCK_GatePWMSM2 };

bool s_sm_init[ 3 ] = { false, false, false };

inline pwm_channels_t sdk_channel( uint8_t channel )
{
    return ( channel == 0 ) ? kPWM_PwmA : kPWM_PwmB;
}

} // namespace

void PwmOut::resolve_pin( int pin )
{
    for ( size_t i = 0; i < sizeof( s_pins ) / sizeof( s_pins[0] ); i++ )
    {
        if ( s_pins[ i ].pin == pin )
        {
            _submodule = s_pins[ i ].submodule;
            _channel   = s_pins[ i ].channel;
            _pin       = pin;
            return;
        }
    }

    _pin = -1;
}

void PwmOut::_acquire_module( void )
{
    if ( _instance_count == 0 )
        RESET_ReleasePeripheralReset( kFLEXPWM0_RST_SHIFT_RSTn );

    _instance_count++;
}

void PwmOut::_release_module( void )
{
    _instance_count--;
}

PwmOut::PwmOut( int pin )
    : Obj( true ), _submodule( 0 ), _channel( 0 ), _pin( -1 ), _period_us( 20000u ), _pulse_us( 0u )
{
    resolve_pin( pin );

    if ( -1 == _pin )
    {
        panic( "PwmOut: unsupported PWM pin" );
        return;
    }

    _acquire_module();
    CLOCK_EnableClock( s_sm_clock[ _submodule ] );

    {
        DigitalInOut pin_io( (uint8_t)pin );
        pin_io.pin_mux( (int)kPORT_MuxAlt5 );   // FlexPWM0 function on this pin
    }

    if ( !s_sm_init[ _submodule ] )
    {
        pwm_config_t cfg;
        PWM_GetDefaultConfig( &cfg );
        cfg.pairOperation   = kPWM_Independent;   // chA/chB run independently (duty), period still shared by HW
        cfg.enableDebugMode = true;
        PWM_Init( FLEXPWM0, (pwm_submodule_t)_submodule, &cfg );

        // Out of reset every FAULTx input is mapped to disable this submodule's
        // outputs (SDK doc: "a reset sets all bits in this field"). No fault
        // pins are wired on this design, so clear the map for both channels.
        PWM_SetupFaultDisableMap( FLEXPWM0, (pwm_submodule_t)_submodule, kPWM_PwmA, kPWM_faultchannel_0, 0u );
        PWM_SetupFaultDisableMap( FLEXPWM0, (pwm_submodule_t)_submodule, kPWM_PwmB, kPWM_faultchannel_0, 0u );

        s_sm_init[ _submodule ] = true;
    }

    // One-time setup of OUTEN/polarity/fault-state for this channel. The
    // frequency passed here is a throwaway placeholder (safe at the default
    // prescale of /1) — apply() immediately below overwrites the period/duty
    // registers with the real default (20ms / 0%) at a correctly chosen
    // prescaler, so the value used here does not need to match _period_us.
    pwm_signal_param_t sig;
    sig.pwmChannel       = sdk_channel( _channel );
    sig.level            = kPWM_HighTrue;
    sig.dutyCyclePercent = 0;
    sig.deadtimeValue    = 0;
    sig.faultState       = kPWM_PwmFaultState0;
    sig.pwmchannelenable = true;
    PWM_SetupPwm( FLEXPWM0, (pwm_submodule_t)_submodule, &sig, 1, kPWM_EdgeAligned, 1000u,
                  CLOCK_GetFreq( kCLOCK_MainClk ) );

    apply();   // sets the real default period/duty and calls PWM_SetPwmLdok()
    PWM_StartTimer( FLEXPWM0, (uint8_t)( 1u << _submodule ) );
}

PwmOut::~PwmOut()
{
    if ( -1 == _pin )
        return;

    _pulse_us = 0;
    apply();

    _release_module();
}

void PwmOut::apply( void )
{
    if ( -1 == _pin )
        return;

    uint32_t src_clk = CLOCK_GetFreq( kCLOCK_MainClk );

    // Pick the smallest prescaler (highest resolution) for which the period
    // still fits the 16-bit tick counter, clamping to the fastest/slowest
    // achievable period at the extremes (memo: clamp to the actual range
    // the FlexPWM prescaler/clock can produce).
    uint8_t  prescale  = 0;
    uint64_t pulseCnt64 = 0;
    for ( prescale = 0; prescale <= 7; prescale++ )
    {
        pulseCnt64 = ( (uint64_t)_period_us * ( src_clk >> prescale ) ) / 1000000ULL;
        if ( pulseCnt64 <= 0xFFFFu || prescale == 7 )
            break;
    }
    if ( pulseCnt64 < 1 )
        pulseCnt64 = 1;
    if ( pulseCnt64 > 0xFFFFu )
        pulseCnt64 = 0xFFFFu;

    uint16_t pulseCnt = (uint16_t)pulseCnt64;

    uint16_t cur_prsc = (uint16_t)( ( FLEXPWM0->SM[ _submodule ].CTRL & PWM_CTRL_PRSC_MASK ) >> PWM_CTRL_PRSC_SHIFT );
    if ( cur_prsc != prescale )
        PWM_SetClockMode( FLEXPWM0, (pwm_submodule_t)_submodule, (pwm_clock_prescale_t)prescale );

    uint64_t dutyTicks = ( (uint64_t)_pulse_us * ( src_clk >> prescale ) ) / 1000000ULL;
    if ( dutyTicks > pulseCnt )
        dutyTicks = pulseCnt;
    uint16_t duty16 = pulseCnt ? (uint16_t)( ( dutyTicks * 65535ULL ) / pulseCnt ) : 0u;

    PWM_UpdatePwmPeriodAndDutycycle( FLEXPWM0, (pwm_submodule_t)_submodule, sdk_channel( _channel ),
                                      kPWM_EdgeAligned, pulseCnt, duty16 );
    PWM_SetPwmLdok( FLEXPWM0, (uint8_t)( 1u << _submodule ), true );
}

void PwmOut::period( float seconds )
{
    if ( -1 == _pin )
        return;

    uint32_t src_clk    = CLOCK_GetFreq( kCLOCK_MainClk );
    float    min_period = 1.0e6f / (float)src_clk;                 // 1 tick @ prescale/1
    float    max_period = 1.0e6f * 65535.0f * 128.0f / (float)src_clk; // 65535 ticks @ prescale/128
    float    period_us  = seconds * 1.0e6f;

    if ( period_us < min_period )
        period_us = min_period;
    if ( period_us > max_period )
        period_us = max_period;

    _period_us = (uint32_t)period_us;
    if ( _pulse_us > _period_us )
        _pulse_us = _period_us;

    apply();
}

void PwmOut::period_ms( int ms ) { period( (float)ms * 1.0e-3f ); }
void PwmOut::period_us( int us ) { period( (float)us * 1.0e-6f ); }

void PwmOut::pulsewidth( float seconds )
{
    if ( -1 == _pin )
        return;

    float pulse_us = seconds * 1.0e6f;
    if ( pulse_us < 0.0f )
        pulse_us = 0.0f;
    if ( pulse_us > (float)_period_us )
        pulse_us = (float)_period_us;

    _pulse_us = (uint32_t)pulse_us;
    apply();
}

void PwmOut::pulsewidth_ms( int ms ) { pulsewidth( (float)ms * 1.0e-3f ); }
void PwmOut::pulsewidth_us( int us ) { pulsewidth( (float)us * 1.0e-6f ); }

void PwmOut::write( float duty )
{
    if ( duty < 0.0f )
        duty = 0.0f;
    if ( duty > 1.0f )
        duty = 1.0f;

    pulsewidth( duty * (float)_period_us * 1.0e-6f );
}

float PwmOut::read( void )
{
    return _period_us ? ( (float)_pulse_us / (float)_period_us ) : 0.0f;
}

PwmOut &PwmOut::operator=( float duty )
{
    write( duty );
    return *this;
}

PwmOut::operator float()
{
    return read();
}

#elif defined( CPU_MCXN947VDF )

extern "C" {
#include "fsl_reset.h"
}

// io.h (included below, via PwmOut.h) redefines PWM1 as a logical Arduino
// pin name, reclaiming it from the SDK's own `PWM1` macro (the FlexPWM1
// instance pointer, (PWM_Type*)PWM1_BASE) -- capture that original SDK
// meaning here, before that redefinition takes effect, so this file's own
// driver calls below keep referring to the actual peripheral.
static PWM_Type * const FLEXPWM1 = PWM1;

#include "PwmOut.h"
#include "mcu.h"

uint8_t PwmOut::_instance_count = 0;

namespace {

struct PwmPinDescriptor {
    int      pin;         // io.h logical pin (PWM0..PWM5)
    uint32_t port_pin;     // PORT2 pin number
    uint8_t  submodule;    // 0=sm0, 1=sm1, 2=sm2 (this chip's FlexPWM1)
    uint8_t  channel;      // 0=chA, 1=chB
    uint8_t  alt;          // PORT mux ALT value (not uniform across pins here)
};

// Physical pins are P2_2..P2_7 on FlexPWM1 (see io.h for why -- P3_6..P3_11
// used earlier were unrouted test points). PWM0..PWM5 assignment below
// matches the "PWM0".."PWM5" silkscreen labels on the Arduino Shield
// Compatible Headers schematic sheet (FRDM-MCXN947SH.pdf page 12), which is
// why it isn't in physical pin order.
//
// ALT is uniformly 5 for all six pins -- confirmed against Zephyr's
// silicon-derived pinctrl header (MCXN947VDF-pinctrl.h, N9X_MUX(port,pin,
// mux) macros), not derived by counting position in pin_mux.c's
// pin_signal string. That position-counting method (used successfully
// elsewhere in this codebase, e.g. I3C1_SDA's Alt10) breaks down for
// P2_2/P2_3 specifically: their pin_signal lists have an extra "CLKOUT"
// entry ahead of PWM1_A2/B2 that doesn't actually consume a mux slot, so
// counting gave Alt6/Alt4 -- both wrong, and both silently produced no
// PWM output at all on real hardware (caught via logic analyzer, no
// signal on any probed pin) before this fix.
const PwmPinDescriptor s_pins[] = {
    //  pin    pin#  sm  ch  alt
    { PWM0,  3u, 2u, 1u, 5u },   // P2_3, PWM1_B2
    { PWM1,  2u, 2u, 0u, 5u },   // P2_2, PWM1_A2
    { PWM2,  5u, 1u, 1u, 5u },   // P2_5, PWM1_B1
    { PWM3,  4u, 1u, 0u, 5u },   // P2_4, PWM1_A1
    { PWM4,  7u, 0u, 1u, 5u },   // P2_7, PWM1_B0
    { PWM5,  6u, 0u, 0u, 5u },   // P2_6, PWM1_A0
};

const clock_ip_name_t s_sm_clock[ 4 ] = { kCLOCK_Pwm1_Sm0, kCLOCK_Pwm1_Sm1, kCLOCK_Pwm1_Sm2, kCLOCK_Pwm1_Sm3 };

bool s_sm_init[ 4 ] = { false, false, false, false };

inline pwm_channels_t sdk_channel( uint8_t channel )
{
    return ( channel == 0 ) ? kPWM_PwmA : kPWM_PwmB;
}

} // namespace

void PwmOut::resolve_pin( int pin )
{
    for ( size_t i = 0; i < sizeof( s_pins ) / sizeof( s_pins[0] ); i++ )
    {
        if ( s_pins[ i ].pin == pin )
        {
            _submodule = s_pins[ i ].submodule;
            _channel   = s_pins[ i ].channel;
            _pin       = pin;

            DigitalInOut pin_io( (uint8_t)pin );
            pin_io.pin_mux( (int)s_pins[ i ].alt );
            return;
        }
    }

    _pin = -1;
}

void PwmOut::_acquire_module( void )
{
    if ( _instance_count == 0 )
        RESET_ReleasePeripheralReset( kPWM1_RST_SHIFT_RSTn );

    _instance_count++;
}

void PwmOut::_release_module( void )
{
    _instance_count--;
}

PwmOut::PwmOut( int pin )
    : Obj( true ), _submodule( 0 ), _channel( 0 ), _pin( -1 ), _period_us( 20000u ), _pulse_us( 0u )
{
    resolve_pin( pin );

    if ( -1 == _pin )
    {
        panic( "PwmOut: unsupported PWM pin" );
        return;
    }

    _acquire_module();
    CLOCK_EnableClock( s_sm_clock[ _submodule ] );

    if ( !s_sm_init[ _submodule ] )
    {
        pwm_config_t cfg;
        PWM_GetDefaultConfig( &cfg );
        cfg.pairOperation   = kPWM_Independent;   // chA/chB run independently (duty), period still shared by HW
        cfg.enableDebugMode = true;
        PWM_Init( FLEXPWM1, (pwm_submodule_t)_submodule, &cfg );

        // Out of reset every FAULTx input is mapped to disable this submodule's
        // outputs. No fault pins are wired on this design, so clear the map
        // for both channels (same requirement as A153).
        PWM_SetupFaultDisableMap( FLEXPWM1, (pwm_submodule_t)_submodule, kPWM_PwmA, kPWM_faultchannel_0, 0u );
        PWM_SetupFaultDisableMap( FLEXPWM1, (pwm_submodule_t)_submodule, kPWM_PwmB, kPWM_faultchannel_0, 0u );

        s_sm_init[ _submodule ] = true;
    }

    // One-time setup of OUTEN/polarity/fault-state for this channel. apply()
    // immediately below overwrites period/duty with the real default (20ms /
    // 0%), so the frequency passed here is just a safe placeholder.
    pwm_signal_param_t sig;
    sig.pwmChannel       = sdk_channel( _channel );
    sig.level            = kPWM_HighTrue;
    sig.dutyCyclePercent = 0;
    sig.deadtimeValue    = 0;
    sig.faultState       = kPWM_PwmFaultState0;
    sig.pwmchannelenable = true;
    PWM_SetupPwm( FLEXPWM1, (pwm_submodule_t)_submodule, &sig, 1, kPWM_EdgeAligned, 1000u,
                  CLOCK_GetFreq( kCLOCK_MainClk ) );

    apply();   // sets the real default period/duty and calls PWM_SetPwmLdok()
    PWM_StartTimer( FLEXPWM1, (uint8_t)( 1u << _submodule ) );
}

PwmOut::~PwmOut()
{
    if ( -1 == _pin )
        return;

    _pulse_us = 0;
    apply();

    _release_module();
}

void PwmOut::apply( void )
{
    if ( -1 == _pin )
        return;

    uint32_t src_clk = CLOCK_GetFreq( kCLOCK_MainClk );

    uint8_t  prescale  = 0;
    uint64_t pulseCnt64 = 0;
    for ( prescale = 0; prescale <= 7; prescale++ )
    {
        pulseCnt64 = ( (uint64_t)_period_us * ( src_clk >> prescale ) ) / 1000000ULL;
        if ( pulseCnt64 <= 0xFFFFu || prescale == 7 )
            break;
    }
    if ( pulseCnt64 < 1 )
        pulseCnt64 = 1;
    if ( pulseCnt64 > 0xFFFFu )
        pulseCnt64 = 0xFFFFu;

    uint16_t pulseCnt = (uint16_t)pulseCnt64;

    uint16_t cur_prsc = (uint16_t)( ( FLEXPWM1->SM[ _submodule ].CTRL & PWM_CTRL_PRSC_MASK ) >> PWM_CTRL_PRSC_SHIFT );
    if ( cur_prsc != prescale )
        PWM_SetClockMode( FLEXPWM1, (pwm_submodule_t)_submodule, (pwm_clock_prescale_t)prescale );

    uint64_t dutyTicks = ( (uint64_t)_pulse_us * ( src_clk >> prescale ) ) / 1000000ULL;
    if ( dutyTicks > pulseCnt )
        dutyTicks = pulseCnt;
    uint16_t duty16 = pulseCnt ? (uint16_t)( ( dutyTicks * 65535ULL ) / pulseCnt ) : 0u;

    PWM_UpdatePwmPeriodAndDutycycle( FLEXPWM1, (pwm_submodule_t)_submodule, sdk_channel( _channel ),
                                      kPWM_EdgeAligned, pulseCnt, duty16 );
    PWM_SetPwmLdok( FLEXPWM1, (uint8_t)( 1u << _submodule ), true );
}

void PwmOut::period( float seconds )
{
    if ( -1 == _pin )
        return;

    uint32_t src_clk    = CLOCK_GetFreq( kCLOCK_MainClk );
    float    min_period = 1.0e6f / (float)src_clk;
    float    max_period = 1.0e6f * 65535.0f * 128.0f / (float)src_clk;
    float    period_us  = seconds * 1.0e6f;

    if ( period_us < min_period )
        period_us = min_period;
    if ( period_us > max_period )
        period_us = max_period;

    _period_us = (uint32_t)period_us;
    if ( _pulse_us > _period_us )
        _pulse_us = _period_us;

    apply();
}

void PwmOut::period_ms( int ms ) { period( (float)ms * 1.0e-3f ); }
void PwmOut::period_us( int us ) { period( (float)us * 1.0e-6f ); }

void PwmOut::pulsewidth( float seconds )
{
    if ( -1 == _pin )
        return;

    float pulse_us = seconds * 1.0e6f;
    if ( pulse_us < 0.0f )
        pulse_us = 0.0f;
    if ( pulse_us > (float)_period_us )
        pulse_us = (float)_period_us;

    _pulse_us = (uint32_t)pulse_us;
    apply();
}

void PwmOut::pulsewidth_ms( int ms ) { pulsewidth( (float)ms * 1.0e-3f ); }
void PwmOut::pulsewidth_us( int us ) { pulsewidth( (float)us * 1.0e-6f ); }

void PwmOut::write( float duty )
{
    if ( duty < 0.0f )
        duty = 0.0f;
    if ( duty > 1.0f )
        duty = 1.0f;

    pulsewidth( duty * (float)_period_us * 1.0e-6f );
}

float PwmOut::read( void )
{
    return _period_us ? ( (float)_pulse_us / (float)_period_us ) : 0.0f;
}

PwmOut &PwmOut::operator=( float duty )
{
    write( duty );
    return *this;
}

PwmOut::operator float()
{
    return read();
}

#endif // defined( CPU_MCXA153VLH ) / defined( CPU_MCXN947VDF )
