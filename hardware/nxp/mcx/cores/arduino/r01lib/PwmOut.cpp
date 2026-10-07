/**
 * @file    PwmOut.cpp
 * @brief   Mbed-compatible PwmOut implementation for r01lib (FlexPWM)
 *
 * Verified against the FRDM-MCXA153 SDK FlexPWM driver example
 * (examples/driver_examples/pwm) and MCXA153_features.h before writing
 * the first version of this file:
 *  - FSL_FEATURE_PWM_FAULT_CH_COUNT is 1, so only `kPWM_faultchannel_0`
 *    (a single DISMAP register per submodule) exists.
 *  - The SDK doc for `PWM_SetupFaultDisableMap()` states "a reset sets all
 *    bits in this field" — i.e. out of reset, *every* FAULTx input is
 *    mapped to disable each channel's output. No fault pins are driven on
 *    these boards, so the disable-map is cleared for both channels of a
 *    submodule the first time it is used, otherwise the PWM output may
 *    never toggle.
 *  - `PWM_SetupPwm()` only supports integer 0-100% duty. For the
 *    continuous-duty Mbed API this file uses the raw-tick API
 *    `PWM_UpdatePwmPeriodAndDutycycle()` (16-bit period/duty) for all
 *    period()/write() calls after the one-time `PWM_SetupPwm()` call that
 *    establishes OUTEN/polarity/fault-state for the channel. The registers
 *    both write are buffered until LDOK, so the placeholder period
 *    `PWM_SetupPwm()` writes never reaches a running channel: apply()
 *    writes the real one before setting LDOK.
 *
 * The period and both channels' pulse widths are kept per submodule
 * (s_sm[] below), not per PwmOut: the two channels share the period
 * register, and apply() rewrites both channels' compare values whenever
 * either changes, so neither is left measured against the other's period.
 *
 * @author  Tedd OKANO
 * @copyright MIT License
 */

#if defined( CPU_MCXA153VLH ) || defined( CPU_MCXA156VLL ) || defined( CPU_MCXN947VDF ) || defined( CPU_MCXN236VDF )

extern "C" {
#include "fsl_reset.h"
}

#if defined( CPU_MCXN947VDF ) || defined( CPU_MCXN236VDF )
// io.h (included below, via PwmOut.h) redefines PWM1 as a logical Arduino
// pin name, reclaiming it from these chips' SDK `PWM1` macro (the FlexPWM1
// instance pointer, (PWM_Type*)PWM1_BASE) -- capture that original SDK
// meaning here, before that redefinition takes effect, so this file's own
// driver calls below keep referring to the actual peripheral. The A15x
// SDKs call their instances FLEXPWM0/FLEXPWM1 and are not affected.
static PWM_Type * const FLEXPWM1 = PWM1;
#endif

#include "PwmOut.h"
#include "mcu.h"
#include "pin_registry.h"

namespace {

struct PwmPinDescriptor {
    int      pin;          // io.h logical pin
    uint8_t  module;       // index into s_module[]
    uint8_t  submodule;    // 0=sm0, 1=sm1, 2=sm2
    uint8_t  channel;      // 0=chA, 1=chB
    uint8_t  alt;          // PORT mux ALT
};

#if defined( CPU_MCXA153VLH )

// FRDM-MCXA153: P3_11..P3_6 on FlexPWM0, ALT5. PWM0..PWM5 run in reverse
// physical-pin order to match the on-board connector orientation.
#define PWM_MODULES         1
#define PWM_SOURCE_CLOCK    kCLOCK_MainClk

PWM_Type * const        s_module[ PWM_MODULES ]       = { FLEXPWM0 };
const reset_ip_name_t   s_reset[ PWM_MODULES ]        = { kFLEXPWM0_RST_SHIFT_RSTn };
const clock_ip_name_t   s_sm_clock[ PWM_MODULES ][ 3 ] = {
    { kCLOCK_GatePWMSM0, kCLOCK_GatePWMSM1, kCLOCK_GatePWMSM2 },
};

const PwmPinDescriptor s_pins[] = {
    //  pin   mod sm  ch  alt
    { PWM5,  0u, 0u, 0u, 5u },   // P3_6,  PWM0_A0
    { PWM4,  0u, 0u, 1u, 5u },   // P3_7,  PWM0_B0
    { PWM3,  0u, 1u, 0u, 5u },   // P3_8,  PWM0_A1
    { PWM2,  0u, 1u, 1u, 5u },   // P3_9,  PWM0_B1
    { PWM1,  0u, 2u, 0u, 5u },   // P3_10, PWM0_A2
    { PWM0,  0u, 2u, 1u, 5u },   // P3_11, PWM0_B2
};

#elif defined( CPU_MCXA156VLL )

// FRDM-MCXA156: PWM0-PWM5 are the same P3_11..P3_6 on FlexPWM0 (ALT5) as
// on FRDM-MCXA153, on the motor-control header J3. D3, D5, D6 and D9 reach
// FlexPWM1 (ALT7, per Zephyr's MCXA156VLL-pinctrl.h), so they are four more
// PWM pins of their own: D6 and D9 share sm0, D5 is sm1 and D3 is sm2.
// Their sm1/sm2 B channels are P3_15/P3_13, D11/D10 on a board as shipped,
// and SPI's lines once R59/R60 are moved over (PIN_MAPPING_A156.md).
#define PWM_MODULES         2
#define PWM_SOURCE_CLOCK    kCLOCK_MainClk

PWM_Type * const        s_module[ PWM_MODULES ]       = { FLEXPWM0, FLEXPWM1 };
const reset_ip_name_t   s_reset[ PWM_MODULES ]        = { kFLEXPWM0_RST_SHIFT_RSTn, kFLEXPWM1_RST_SHIFT_RSTn };
const clock_ip_name_t   s_sm_clock[ PWM_MODULES ][ 3 ] = {
    { kCLOCK_GatePWM0SM0, kCLOCK_GatePWM0SM1, kCLOCK_GatePWM0SM2 },
    { kCLOCK_GatePWM1SM0, kCLOCK_GatePWM1SM1, kCLOCK_GatePWM1SM2 },
};

const PwmPinDescriptor s_pins[] = {
    //  pin   mod sm  ch  alt
    { PWM5,  0u, 0u, 0u, 5u },   // P3_6,  PWM0_A0
    { PWM4,  0u, 0u, 1u, 5u },   // P3_7,  PWM0_B0
    { PWM3,  0u, 1u, 0u, 5u },   // P3_8,  PWM0_A1
    { PWM2,  0u, 1u, 1u, 5u },   // P3_9,  PWM0_B1
    { PWM1,  0u, 2u, 0u, 5u },   // P3_10, PWM0_A2
    { PWM0,  0u, 2u, 1u, 5u },   // P3_11, PWM0_B2
    { D6,    1u, 0u, 0u, 7u },   // P3_16, PWM1_A0
    { D9,    1u, 0u, 1u, 7u },   // P3_17, PWM1_B0
    { D5,    1u, 1u, 0u, 7u },   // P3_14, PWM1_A1
    { D3,    1u, 2u, 0u, 7u },   // P3_12, PWM1_A2 (also the red LED)
};

#elif defined( CPU_MCXN947VDF )

// FRDM-MCXN947: P2_2..P2_7 on FlexPWM1 (not FlexPWM0), ALT5. The PWM0..PWM5
// assignment matches the "PWM0".."PWM5" silkscreen labels on the Arduino
// Shield Compatible Headers sheet of the schematic (FRDM-MCXN947SH.pdf
// page 12), which is why it isn't in physical pin order.
//
// Corrected twice after real-hardware bring-up: an earlier version reused
// A153's P3_6..P3_11, which on N947 are unrouted test points; and the first
// fix derived ALT by counting position in pin_mux.c's pin_signal strings,
// which gave Alt6/Alt4 for P2_2/P2_3 (an extra "CLKOUT" entry there doesn't
// consume a mux slot) and silently produced no PWM output at all. ALT 5 for
// all six is from Zephyr's silicon-derived MCXN947VDF-pinctrl.h.
#define PWM_MODULES         1
#define PWM_SOURCE_CLOCK    kCLOCK_MainClk

PWM_Type * const        s_module[ PWM_MODULES ]       = { FLEXPWM1 };
const reset_ip_name_t   s_reset[ PWM_MODULES ]        = { kPWM1_RST_SHIFT_RSTn };
const clock_ip_name_t   s_sm_clock[ PWM_MODULES ][ 3 ] = {
    { kCLOCK_Pwm1_Sm0, kCLOCK_Pwm1_Sm1, kCLOCK_Pwm1_Sm2 },
};

const PwmPinDescriptor s_pins[] = {
    //  pin   mod sm  ch  alt
    { PWM0,  0u, 2u, 1u, 5u },   // P2_3, PWM1_B2
    { PWM1,  0u, 2u, 0u, 5u },   // P2_2, PWM1_A2
    { PWM2,  0u, 1u, 1u, 5u },   // P2_5, PWM1_B1
    { PWM3,  0u, 1u, 0u, 5u },   // P2_4, PWM1_A1
    { PWM4,  0u, 0u, 1u, 5u },   // P2_7, PWM1_B0
    { PWM5,  0u, 0u, 0u, 5u },   // P2_6, PWM1_A0
};

#elif defined( CPU_MCXN236VDF )

// FRDM-MCXN236: J3's six FlexPWM1 pins, ALT5 for all six per Zephyr's
// MCXN236VDF-pinctrl.h. Four of them are D-pins too. FlexPWM runs on the
// bus clock, 150MHz as on N947; this chip's SDK has no kCLOCK_MainClk, and
// its own PWM example asks for kCLOCK_BusClk.
#define PWM_MODULES         1
#define PWM_SOURCE_CLOCK    kCLOCK_BusClk

PWM_Type * const        s_module[ PWM_MODULES ]       = { FLEXPWM1 };
const reset_ip_name_t   s_reset[ PWM_MODULES ]        = { kPWM1_RST_SHIFT_RSTn };
const clock_ip_name_t   s_sm_clock[ PWM_MODULES ][ 3 ] = {
    { kCLOCK_Pwm1_Sm0, kCLOCK_Pwm1_Sm1, kCLOCK_Pwm1_Sm2 },
};

const PwmPinDescriptor s_pins[] = {
    //  pin   mod sm  ch  alt
    { PWM0,  0u, 2u, 1u, 5u },   // P3_17, PWM1_B2 (D6)
    { PWM1,  0u, 2u, 0u, 5u },   // P3_16, PWM1_A2
    { PWM2,  0u, 1u, 1u, 5u },   // P3_15, PWM1_B1
    { PWM3,  0u, 1u, 0u, 5u },   // P3_14, PWM1_A1 (D9)
    { PWM4,  0u, 0u, 1u, 5u },   // P2_7,  PWM1_B0 (D5)
    { PWM5,  0u, 0u, 0u, 5u },   // P3_12, PWM1_A0 (D3)
};

#else
#error "PwmOut.cpp: no PWM outputs for this chip"
#endif

// Per-submodule state, shared by the PwmOut on each of its two channels
struct SubmoduleState {
    bool     init;          // PWM_Init() and the fault map done
    uint8_t  users;         // bit 0 = chA has a PwmOut, bit 1 = chB
    uint32_t period_us;
    uint32_t pulse_us[ 2 ];
};

SubmoduleState  s_sm[ PWM_MODULES ][ 3 ]  = {};
uint8_t         s_module_users[ PWM_MODULES ] = {};

inline pwm_channels_t sdk_channel( uint8_t channel )
{
    return ( channel == 0 ) ? kPWM_PwmA : kPWM_PwmB;
}

const PwmPinDescriptor *find_pin( int pin )
{
    for ( size_t i = 0; i < sizeof( s_pins ) / sizeof( s_pins[0] ); i++ )
    {
        if ( s_pins[ i ].pin == pin )
            return &s_pins[ i ];
    }
    return nullptr;
}

} // namespace

bool PwmOut::is_pwm_pin( int pin )
{
    return find_pin( pin ) != nullptr;
}

bool PwmOut::shares_running_period( int pin )
{
    const PwmPinDescriptor *d = find_pin( pin );

    return d && s_sm[ d->module ][ d->submodule ].users;
}

PwmOut::PwmOut( int pin )
    : Obj( true ), _module( 0 ), _submodule( 0 ), _channel( 0 ), _alt( 0 ), _pin( -1 )
{
    const PwmPinDescriptor *d = find_pin( pin );

    if ( nullptr == d )
    {
        panic( "PwmOut: unsupported PWM pin" );
        return;
    }

    _module    = d->module;
    _submodule = d->submodule;
    _channel   = d->channel;
    _alt       = d->alt;
    _pin       = pin;

    PWM_Type       *base = s_module[ _module ];
    SubmoduleState &sm   = s_sm[ _module ][ _submodule ];

    if ( 0 == s_module_users[ _module ]++ )
        RESET_ReleasePeripheralReset( s_reset[ _module ] );

    CLOCK_EnableClock( s_sm_clock[ _module ][ _submodule ] );

    {
        DigitalInOut pin_io( (uint8_t)pin );
        pin_io.pin_mux( (int)_alt );
    }

    if ( !sm.init )
    {
        pwm_config_t cfg;
        PWM_GetDefaultConfig( &cfg );
        cfg.pairOperation   = kPWM_Independent;   // chA/chB run independently (duty), period still shared by HW
        cfg.enableDebugMode = true;
        PWM_Init( base, (pwm_submodule_t)_submodule, &cfg );

        PWM_SetupFaultDisableMap( base, (pwm_submodule_t)_submodule, kPWM_PwmA, kPWM_faultchannel_0, 0u );
        PWM_SetupFaultDisableMap( base, (pwm_submodule_t)_submodule, kPWM_PwmB, kPWM_faultchannel_0, 0u );

        sm.init = true;
    }

    // The first PwmOut on the submodule sets the Mbed default of 20ms; one
    // made next to a running channel takes over its period
    if ( !sm.users )
        sm.period_us = 20000u;

    sm.users                 |= (uint8_t)( 1u << _channel );
    sm.pulse_us[ _channel ]   = 0u;

    // One-time setup of OUTEN/polarity/fault-state for this channel. The
    // period it writes is a placeholder that apply() replaces before LDOK.
    pwm_signal_param_t sig;
    sig.pwmChannel       = sdk_channel( _channel );
    sig.level            = kPWM_HighTrue;
    sig.dutyCyclePercent = 0;
    sig.deadtimeValue    = 0;
    sig.faultState       = kPWM_PwmFaultState0;
    sig.pwmchannelenable = true;
    PWM_SetupPwm( base, (pwm_submodule_t)_submodule, &sig, 1, kPWM_EdgeAligned, 1000u,
                  CLOCK_GetFreq( PWM_SOURCE_CLOCK ) );

    apply();   // the real period and duty, then PWM_SetPwmLdok()
    PWM_StartTimer( base, (uint8_t)( 1u << _submodule ) );

    uint8_t pin8 = (uint8_t)_pin;
    pin_registry_note( this, "PwmOut", &pin8, 1, _alt );
}

PwmOut::~PwmOut()
{
    pin_registry_forget( this );

    if ( -1 == _pin )
        return;

    SubmoduleState &sm = s_sm[ _module ][ _submodule ];

    sm.pulse_us[ _channel ] = 0u;
    apply();
    sm.users &= (uint8_t)~( 1u << _channel );

    s_module_users[ _module ]--;
}

void PwmOut::claim_pin( void )
{
    if ( -1 == _pin )
        return;

    {
        DigitalInOut pin_io( (uint8_t)_pin );
        pin_io.pin_mux( (int)_alt );
    }

    uint8_t pin8 = (uint8_t)_pin;
    pin_registry_note( this, "PwmOut", &pin8, 1, _alt );
}

void PwmOut::release_pin( void )
{
    pin_registry_forget( this );
}

void PwmOut::apply( void )
{
    if ( -1 == _pin )
        return;

    PWM_Type       *base    = s_module[ _module ];
    SubmoduleState &sm      = s_sm[ _module ][ _submodule ];
    uint32_t        src_clk = CLOCK_GetFreq( PWM_SOURCE_CLOCK );

    // Pick the smallest prescaler (highest resolution) for which the period
    // still fits the 16-bit tick counter, clamping to the fastest/slowest
    // achievable period at the extremes.
    uint8_t  prescale   = 0;
    uint64_t pulseCnt64 = 0;
    for ( prescale = 0; prescale <= 7; prescale++ )
    {
        pulseCnt64 = ( (uint64_t)sm.period_us * ( src_clk >> prescale ) ) / 1000000ULL;
        if ( pulseCnt64 <= 0xFFFFu || prescale == 7 )
            break;
    }
    if ( pulseCnt64 < 1 )
        pulseCnt64 = 1;
    if ( pulseCnt64 > 0xFFFFu )
        pulseCnt64 = 0xFFFFu;

    uint16_t pulseCnt = (uint16_t)pulseCnt64;

    uint16_t cur_prsc = (uint16_t)( ( base->SM[ _submodule ].CTRL & PWM_CTRL_PRSC_MASK ) >> PWM_CTRL_PRSC_SHIFT );
    if ( cur_prsc != prescale )
        PWM_SetClockMode( base, (pwm_submodule_t)_submodule, (pwm_clock_prescale_t)prescale );

    // Both channels, so the one this PwmOut isn't on keeps its pulse width
    // against the period it now shares
    for ( uint8_t ch = 0; ch < 2; ch++ )
    {
        if ( !( sm.users & ( 1u << ch ) ) )
            continue;

        uint64_t dutyTicks = ( (uint64_t)sm.pulse_us[ ch ] * ( src_clk >> prescale ) ) / 1000000ULL;
        if ( dutyTicks > pulseCnt )
            dutyTicks = pulseCnt;
        uint16_t duty16 = (uint16_t)( ( dutyTicks * 65535ULL ) / pulseCnt );

        PWM_UpdatePwmPeriodAndDutycycle( base, (pwm_submodule_t)_submodule, sdk_channel( ch ),
                                          kPWM_EdgeAligned, pulseCnt, duty16 );
    }
    PWM_SetPwmLdok( base, (uint8_t)( 1u << _submodule ), true );
}

void PwmOut::period( float seconds )
{
    if ( -1 == _pin )
        return;

    SubmoduleState &sm         = s_sm[ _module ][ _submodule ];
    uint32_t        src_clk    = CLOCK_GetFreq( PWM_SOURCE_CLOCK );
    float           min_period = 1.0e6f / (float)src_clk;                    // 1 tick @ prescale/1
    float           max_period = 1.0e6f * 65535.0f * 128.0f / (float)src_clk; // 65535 ticks @ prescale/128
    float           period_us  = seconds * 1.0e6f;

    if ( period_us < min_period )
        period_us = min_period;
    if ( period_us > max_period )
        period_us = max_period;

    sm.period_us = (uint32_t)period_us;
    for ( uint8_t ch = 0; ch < 2; ch++ )
    {
        if ( sm.pulse_us[ ch ] > sm.period_us )
            sm.pulse_us[ ch ] = sm.period_us;
    }

    apply();
}

void PwmOut::period_ms( int ms ) { period( (float)ms * 1.0e-3f ); }
void PwmOut::period_us( int us ) { period( (float)us * 1.0e-6f ); }

void PwmOut::pulsewidth( float seconds )
{
    if ( -1 == _pin )
        return;

    SubmoduleState &sm       = s_sm[ _module ][ _submodule ];
    float           pulse_us = seconds * 1.0e6f;

    if ( pulse_us < 0.0f )
        pulse_us = 0.0f;
    if ( pulse_us > (float)sm.period_us )
        pulse_us = (float)sm.period_us;

    sm.pulse_us[ _channel ] = (uint32_t)pulse_us;
    apply();
}

void PwmOut::pulsewidth_ms( int ms ) { pulsewidth( (float)ms * 1.0e-3f ); }
void PwmOut::pulsewidth_us( int us ) { pulsewidth( (float)us * 1.0e-6f ); }

void PwmOut::write( float duty )
{
    if ( -1 == _pin )
        return;

    if ( duty < 0.0f )
        duty = 0.0f;
    if ( duty > 1.0f )
        duty = 1.0f;

    pulsewidth( duty * (float)s_sm[ _module ][ _submodule ].period_us * 1.0e-6f );
}

float PwmOut::read( void )
{
    if ( -1 == _pin )
        return 0.0f;

    const SubmoduleState &sm = s_sm[ _module ][ _submodule ];

    return sm.period_us ? ( (float)sm.pulse_us[ _channel ] / (float)sm.period_us ) : 0.0f;
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

#else
#error "PwmOut.cpp: no PWM outputs for this chip"
#endif
