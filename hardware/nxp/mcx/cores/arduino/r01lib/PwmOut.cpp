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
 *  - `PWM_SetupPwm()` only supports integer 0-100% duty. It is called once
 *    per channel, to set up OUTEN/polarity/fault-state; after that, apply()
 *    writes the period and compare registers itself, in ticks (the
 *    registers `PWM_UpdatePwmPeriodAndDutycycle()` writes, without its
 *    16-bit duty fraction). The registers are buffered until LDOK, so the
 *    placeholder period `PWM_SetupPwm()` writes never reaches a running
 *    channel: apply() writes the real one before setting LDOK.
 *
 * The period and both channels' pulse widths are kept per submodule
 * (s_sm[] below), not per PwmOut: the two channels share the period
 * register, and apply() rewrites both channels' compare values whenever
 * either changes, so neither is left measured against the other's period.
 * They are kept in ticks of the source clock, not in microseconds, so the
 * duty has as many steps as the counter at any frequency, and a frequency
 * is set to the nearest tick (0.12.0; before, 8 bits held only up to about
 * 3.9kHz, and a frequency was set to the nearest whole microsecond).
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

// Per-submodule state, shared by the PwmOut on each of its two channels.
// Times are in ticks of the source clock, before the prescaler, so the
// duty has as many steps as the hardware can make at any frequency
struct SubmoduleState {
    bool     init;          // PWM_Init() and the fault map done
    uint8_t  users;         // bit 0 = chA has a PwmOut, bit 1 = chB
    uint32_t period_ticks;
    uint32_t pulse_ticks[ 2 ];
};

SubmoduleState  s_sm[ PWM_MODULES ][ 3 ]  = {};
uint8_t         s_module_users[ PWM_MODULES ] = {};

constexpr uint32_t  MIN_TICKS   = 1u;               // 1 tick @ prescale/1
constexpr uint32_t  MAX_TICKS   = 0xFFFFu << 7;     // 65535 ticks @ prescale/128

inline pwm_channels_t sdk_channel( uint8_t channel )
{
    return ( channel == 0 ) ? kPWM_PwmA : kPWM_PwmB;
}

inline uint32_t source_clock( void )
{
    return CLOCK_GetFreq( PWM_SOURCE_CLOCK );
}

// Source-clock ticks counted with a prescaler of 2^prescale, rounded
inline uint32_t ticks_at( uint32_t ticks, uint8_t prescale )
{
    return (uint32_t)( ( (uint64_t)ticks + ( ( 1u << prescale ) >> 1 ) ) >> prescale );
}

uint32_t us_to_ticks( int64_t us )
{
    if ( us <= 0 )
        return 0u;

    uint64_t ticks = ( (uint64_t)us * source_clock() + 500000u ) / 1000000u;

    return ( ticks > 0xFFFFFFFFu ) ? 0xFFFFFFFFu : (uint32_t)ticks;
}

uint32_t seconds_to_ticks( float seconds )
{
    float ticks = seconds * (float)source_clock() + 0.5f;

    if ( !( ticks >= 1.0f ) )   // NaN too
        return 0u;
    if ( ticks >= 4294967040.0f )   // the largest float below 2^32
        return 0xFFFFFFFFu;

    return (uint32_t)ticks;
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
        sm.period_ticks = us_to_ticks( 20000 );

    sm.users                    |= (uint8_t)( 1u << _channel );
    sm.pulse_ticks[ _channel ]   = 0u;

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

    sm.pulse_ticks[ _channel ] = 0u;
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

    // Pick the smallest prescaler (highest resolution) for which the period
    // still fits the 16-bit tick counter. period() keeps it within
    // MIN_TICKS..MAX_TICKS, so the count is 1..0xFFFF.
    uint8_t  prescale = 0;
    uint32_t count    = 0;
    for ( prescale = 0; prescale <= 7; prescale++ )
    {
        count = ticks_at( sm.period_ticks, prescale );
        if ( count <= 0xFFFFu || prescale == 7 )
            break;
    }
    if ( count < 1 )
        count = 1;
    if ( count > 0xFFFFu )
        count = 0xFFFFu;

    uint16_t cur_prsc = (uint16_t)( ( base->SM[ _submodule ].CTRL & PWM_CTRL_PRSC_MASK ) >> PWM_CTRL_PRSC_SHIFT );
    if ( cur_prsc != prescale )
        PWM_SetClockMode( base, (pwm_submodule_t)_submodule, (pwm_clock_prescale_t)prescale );

    // Edge-aligned, as PWM_SetupPwm() set the channel up: the counter runs
    // from INIT = 0 to VAL1, and a channel is high from VAL2 (A) or VAL4
    // (B), 0, until VAL3 or VAL5. A pulse as long as the period never
    // reaches its VAL3/VAL5 and stays high. These are the registers the
    // SDK's PWM_UpdatePwmPeriodAndDutycycle() writes, here in ticks: that
    // function takes the duty as a 16-bit fraction and turns it back into
    // ticks, which loses steps. Like it, the values wait for LDOK.
    base->SM[ _submodule ].INIT = 0u;
    base->SM[ _submodule ].VAL0 = (uint16_t)( count / 2u );
    base->SM[ _submodule ].VAL1 = (uint16_t)( count - 1u );

    // Both channels, so the one this PwmOut isn't on keeps its pulse width
    // against the period it now shares
    for ( uint8_t ch = 0; ch < 2; ch++ )
    {
        if ( !( sm.users & ( 1u << ch ) ) )
            continue;

        uint32_t high = ticks_at( sm.pulse_ticks[ ch ], prescale );
        if ( sm.pulse_ticks[ ch ] >= sm.period_ticks || high > count )
            high = count;

        if ( ch == 0 )
        {
            base->SM[ _submodule ].VAL2 = 0u;
            base->SM[ _submodule ].VAL3 = (uint16_t)high;
        }
        else
        {
            base->SM[ _submodule ].VAL4 = 0u;
            base->SM[ _submodule ].VAL5 = (uint16_t)high;
        }
    }
    PWM_SetPwmLdok( base, (uint8_t)( 1u << _submodule ), true );
}

void PwmOut::set_period( uint32_t ticks )
{
    if ( -1 == _pin )
        return;

    SubmoduleState &sm = s_sm[ _module ][ _submodule ];

    if ( ticks < MIN_TICKS )
        ticks = MIN_TICKS;
    if ( ticks > MAX_TICKS )
        ticks = MAX_TICKS;

    sm.period_ticks = ticks;
    for ( uint8_t ch = 0; ch < 2; ch++ )
    {
        if ( sm.pulse_ticks[ ch ] > sm.period_ticks )
            sm.pulse_ticks[ ch ] = sm.period_ticks;
    }

    apply();
}

void PwmOut::set_pulse( uint32_t ticks )
{
    if ( -1 == _pin )
        return;

    SubmoduleState &sm = s_sm[ _module ][ _submodule ];

    sm.pulse_ticks[ _channel ] = ( ticks > sm.period_ticks ) ? sm.period_ticks : ticks;
    apply();
}

void PwmOut::period( float seconds ) { set_period( seconds_to_ticks( seconds ) ); }
void PwmOut::period_ms( int ms )     { set_period( us_to_ticks( (int64_t)ms * 1000 ) ); }
void PwmOut::period_us( int us )     { set_period( us_to_ticks( us ) ); }

void PwmOut::frequency( uint32_t hz )
{
    if ( hz < 1u )
        hz = 1u;

    set_period( ( source_clock() + hz / 2u ) / hz );
}

void PwmOut::pulsewidth( float seconds ) { set_pulse( seconds_to_ticks( seconds ) ); }
void PwmOut::pulsewidth_ms( int ms )     { set_pulse( us_to_ticks( (int64_t)ms * 1000 ) ); }
void PwmOut::pulsewidth_us( int us )     { set_pulse( us_to_ticks( us ) ); }

void PwmOut::write( float duty )
{
    if ( -1 == _pin )
        return;

    if ( !( duty > 0.0f ) )     // NaN too
        duty = 0.0f;
    if ( duty > 1.0f )
        duty = 1.0f;

    set_pulse( (uint32_t)( duty * (float)s_sm[ _module ][ _submodule ].period_ticks + 0.5f ) );
}

void PwmOut::write_ratio( uint32_t num, uint32_t den )
{
    if ( -1 == _pin )
        return;

    uint32_t period = s_sm[ _module ][ _submodule ].period_ticks;

    if ( den == 0u )
        num = 0u;
    else if ( num > den )
        num = den;

    set_pulse( den ? (uint32_t)( ( (uint64_t)period * num + den / 2u ) / den ) : 0u );
}

float PwmOut::read( void )
{
    if ( -1 == _pin )
        return 0.0f;

    const SubmoduleState &sm = s_sm[ _module ][ _submodule ];

    return sm.period_ticks ? ( (float)sm.pulse_ticks[ _channel ] / (float)sm.period_ticks ) : 0.0f;
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

#elif defined( CPU_MCXC444VLH )

#include "PwmOut.h"
#include "mcu.h"
#include "pin_registry.h"

unsigned long micros( void );   // arduino_main.cpp

namespace {

struct TpmPin {
    int      pin;
    uint8_t  tpm;
    uint8_t  channel;
    uint8_t  alt;
};

// Zephyr's MCXC444VLH-pinctrl.h
const TpmPin s_pins[] = {
    { D3,  1, 0, 3 },   // PTA12  TPM1_CH0
    { D5,  0, 2, 3 },   // PTA5   TPM0_CH2
    { D6,  0, 2, 3 },   // PTE29  TPM0_CH2
    { D9,  0, 2, 4 },   // PTD2   TPM0_CH2
    { D10, 0, 4, 4 },   // PTD4   TPM0_CH4
};

constexpr int   TPMS        = 2;
constexpr int   CHANNELS    = 6;

TPM_Type *const s_tpm[ TPMS ] = { TPM0, TPM1 };

// Per TPM: the shared period, which PwmOut drives each channel, and the
// MOD and CnV values on their way to the hardware (set_mod(), set_cnv())
constexpr uint8_t   MOD_BIT     = 1u << CHANNELS;  // in waiting: MOD has a value in mod_later

struct TpmState {
    bool     init;
    uint32_t period_ticks;          // in ticks of the source clock, before the prescaler
    PwmOut   *owner[ CHANNELS ];
    uint32_t mod;                   // MOD as last written
    uint32_t mod_free_at;           // micros() when that write has left the MOD buffer
    uint32_t mod_later;             // a MOD that has to wait for the buffer
    uint32_t mod_before;            // the MOD before that, which may still be counting
    uint32_t mod_before_until;      // micros() when it surely isn't any more
    uint16_t cnv[ CHANNELS ];       // CnV as last written
    uint32_t free_at[ CHANNELS ];   // micros() when that write has left the CnV buffer
    uint16_t later[ CHANNELS ];     // a CnV that has to wait for the buffer
    uint8_t  waiting;               // channels with a value in later[], and MOD_BIT
};

TpmState    s_state[ TPMS ] = {};

volatile bool   s_any_waiting   = false;

const TpmPin *find_pin( int pin )
{
    for ( size_t i = 0; i < sizeof( s_pins ) / sizeof( s_pins[0] ); i++ )
    {
        if ( s_pins[ i ].pin == pin )
            return &s_pins[ i ];
    }
    return nullptr;
}

bool running( const TpmState &st )
{
    for ( int ch = 0; ch < CHANNELS; ch++ )
    {
        if ( st.owner[ ch ] )
            return true;
    }
    return false;
}

inline uint32_t source_clock( void )
{
    return CLOCK_GetFreq( kCLOCK_McgIrc48MClk );   // TPMSRC = 1, set below
}

constexpr uint32_t  MIN_TICKS   = 2u;               // 2 ticks @ prescale/1
constexpr uint32_t  MAX_TICKS   = 0xFFFFu << 7;     // 65535 ticks @ prescale/128

// Source-clock ticks counted with a prescaler of 2^prescale, rounded
inline uint32_t ticks_at( uint32_t ticks, uint8_t prescale )
{
    return (uint32_t)( ( (uint64_t)ticks + ( ( 1u << prescale ) >> 1 ) ) >> prescale );
}

uint32_t us_to_ticks( int64_t us )
{
    if ( us <= 0 )
        return 0u;

    uint64_t ticks = ( (uint64_t)us * source_clock() + 500000u ) / 1000000u;

    return ( ticks > 0xFFFFFFFFu ) ? 0xFFFFFFFFu : (uint32_t)ticks;
}

uint32_t seconds_to_ticks( float seconds )
{
    float ticks = seconds * (float)source_clock() + 0.5f;

    if ( !( ticks >= 1.0f ) )   // NaN too
        return 0u;
    if ( ticks >= 4294967040.0f )   // the largest float below 2^32
        return 0xFFFFFFFFu;

    return (uint32_t)ticks;
}

// At most how long until the counter next goes from MOD to 0, when the
// buffers hand their values on, in microseconds, rounded up. An upper
// bound, not a measurement: MOD reads back as last written, which counts
// only from the next overflow, so for a period after a MOD write the old
// one may still be running; and CNT, read just after it was cleared with
// the counter stopped, can still show the old count. A count above MOD
// runs on to 0xFFFF and wraps first. Getting this short leaves a write in
// a buffer that is still taken, and the TPM ignores it
uint32_t us_to_overflow( int t, uint32_t now )
{
    TPM_Type    *base   = s_tpm[ t ];
    TpmState    &st     = s_state[ t ];
    uint32_t    ps      = ( base->SC & TPM_SC_PS_MASK ) >> TPM_SC_PS_SHIFT;
    uint32_t    mod     = st.mod;
    uint32_t    cnt     = base->CNT;

    if ( (int32_t)( now - st.mod_before_until ) < 0 && st.mod_before > mod )
        mod = st.mod_before;

    uint32_t    ticks   = ( cnt <= mod ) ? mod + 1u - cnt : 0x10000u - cnt + mod + 1u;

    return ( ( ticks << ps ) + ( source_clock() / 1000000u ) - 1u ) / ( source_clock() / 1000000u );
}

void write_cnv( int t, int ch, uint16_t value, uint32_t now )
{
    s_tpm[ t ]->CONTROLS[ ch ].CnV  = value;
    s_state[ t ].cnv[ ch ]          = value;
    s_state[ t ].free_at[ ch ]      = now + us_to_overflow( t, now ) + 2u;
}

void write_mod( int t, uint32_t value, uint32_t now )
{
    TpmState    &st     = s_state[ t ];
    uint32_t    until   = now + us_to_overflow( t, now ) + 2u;

    s_tpm[ t ]->MOD     = value;
    st.mod_before       = st.mod;
    st.mod_before_until = until;
    st.mod              = value;
    st.mod_free_at      = until;
}

// MOD is buffered as CnV is (below), and the same way round
void set_mod( int t, uint32_t value, uint32_t now )
{
    TPM_Type    *base   = s_tpm[ t ];
    TpmState    &st     = s_state[ t ];
    uint32_t    primask = __get_PRIMASK();

    __disable_irq();

    if ( !( base->SC & TPM_SC_CMOD_MASK ) )
    {
        // Stopped: the register takes the value at once
        base->MOD           = value;
        st.mod              = value;
        st.mod_free_at      = now;
        st.mod_before       = value;
        st.mod_before_until = now;
        st.waiting          &= (uint8_t)~MOD_BIT;
    }
    else if ( st.waiting & MOD_BIT )
    {
        st.mod_later    = value;            // replaces the value waiting
    }
    else if ( value == st.mod )
    {
        // already there, or on its way
    }
    else if ( (int32_t)( now - st.mod_free_at ) < 0 )
    {
        st.mod_later    = value;
        st.waiting      |= MOD_BIT;
        s_any_waiting   = true;
    }
    else
    {
        write_mod( t, value, now );
    }

    __set_PRIMASK( primask );
}

// While the counter runs, a CnV write waits in a buffer for the end of the
// period, and further writes to that buffer before then are ignored
// (reference manual, TPMx_CnV; seen on the board too). The TPM's own flag
// for the end of a period, TOF, could not be cleared reliably on the board,
// so the time the buffer frees up is worked out from the counter instead,
// and a value for a channel whose buffer is still taken waits here until
// pwm_tpm_service() (SysTick, every millisecond) writes it. analogWrite()
// never waits, and the last value always lands
void set_cnv( int t, int ch, uint16_t value, uint32_t now )
{
    TPM_Type    *base   = s_tpm[ t ];
    TpmState    &st     = s_state[ t ];
    uint8_t     bit     = (uint8_t)( 1u << ch );
    uint32_t    primask = __get_PRIMASK();

    __disable_irq();

    if ( !( base->SC & TPM_SC_CMOD_MASK ) )
    {
        // Stopped: the register takes the value at once
        base->CONTROLS[ ch ].CnV    = value;
        st.cnv[ ch ]                = value;
        st.free_at[ ch ]            = now;
        st.waiting                  &= (uint8_t)~bit;
    }
    else if ( st.waiting & bit )
    {
        st.later[ ch ]  = value;            // replaces the value waiting
    }
    else if ( value == st.cnv[ ch ] )
    {
        // already there, or on its way
    }
    else if ( (int32_t)( now - st.free_at[ ch ] ) < 0 )
    {
        st.later[ ch ]  = value;
        st.waiting      |= bit;
        s_any_waiting   = true;
    }
    else
    {
        write_cnv( t, ch, value, now );
    }

    __set_PRIMASK( primask );
}

} // namespace

void pwm_tpm_service( uint32_t now )
{
    if ( !s_any_waiting )
        return;

    bool    still   = false;

    for ( int t = 0; t < TPMS; t++ )
    {
        TpmState    &st = s_state[ t ];

        if ( st.waiting & MOD_BIT )
        {
            if ( (int32_t)( now - st.mod_free_at ) >= 0 )
            {
                write_mod( t, st.mod_later, now );
                st.waiting  &= (uint8_t)~MOD_BIT;
            }
            else
                still   = true;
        }

        for ( int ch = 0; ch < CHANNELS; ch++ )
        {
            uint8_t bit = (uint8_t)( 1u << ch );

            if ( !( st.waiting & bit ) )
                continue;

            if ( (int32_t)( now - st.free_at[ ch ] ) >= 0 )
            {
                write_cnv( t, ch, st.later[ ch ], now );
                st.waiting  &= (uint8_t)~bit;
            }
            else
                still   = true;
        }
    }

    s_any_waiting   = still;
}

bool PwmOut::is_pwm_pin( int pin )
{
    return find_pin( pin ) != nullptr;
}

bool PwmOut::shares_running_period( int pin )
{
    const TpmPin *d = find_pin( pin );

    return d && running( s_state[ d->tpm ] );
}

PwmOut::PwmOut( int pin )
    : Obj( true ), _tpm( 0 ), _channel( 0 ), _alt( 0 ), _holds_pin( false ), _pulse_ticks( 0 ), _pin( -1 )
{
    const TpmPin *d = find_pin( pin );

    if ( nullptr == d )
    {
        panic( "PwmOut: unsupported PWM pin" );
        return;
    }

    _tpm        = d->tpm;
    _channel    = d->channel;
    _alt        = d->alt;
    _pin        = pin;

    TpmState    &st = s_state[ _tpm ];

    if ( !st.init )
    {
        CLOCK_SetTpmClock( 1u );    // IRC48M, as the SDK's TPM examples do

        tpm_config_t cfg;
        TPM_GetDefaultConfig( &cfg );
        TPM_Init( s_tpm[ _tpm ], &cfg );    // also turns the TPM's clock on
        st.init = true;
    }

    // The first PwmOut on a TPM sets the Mbed default of 20ms; one made next
    // to a running channel takes over the TPM's period
    if ( !running( st ) )
        st.period_ticks = us_to_ticks( 20000 );

    take_channel();
}

PwmOut::~PwmOut()
{
    pin_registry_forget( this );

    if ( -1 == _pin )
        return;

    TpmState    &st = s_state[ _tpm ];

    if ( st.owner[ _channel ] == this )
    {
        _pulse_ticks    = 0u;
        apply();
        st.owner[ _channel ] = nullptr;
    }
}

void PwmOut::take_channel( void )
{
    TpmState    &st     = s_state[ _tpm ];
    PwmOut      *prev   = st.owner[ _channel ];

    //  D5, D6 and D9 share TPM0_CH2: the pin that had it goes back to GPIO,
    //  driven LOW, unless something else has had it since
    if ( prev && prev != this && prev->_holds_pin )
    {
        DigitalInOut    low( (uint8_t)prev->_pin, kGPIO_DigitalOutput, 0 );
        prev->_holds_pin    = false;
        pin_registry_forget( prev );
    }

    st.owner[ _channel ]    = this;

    // Edge-aligned PWM, high-true
    s_tpm[ _tpm ]->CONTROLS[ _channel ].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;

    {
        DigitalInOut pin_io( (uint8_t)_pin );
        pin_io.pin_mux( (int)_alt );
    }
    _holds_pin  = true;

    apply();

    uint8_t pin8 = (uint8_t)_pin;
    pin_registry_note( this, "PwmOut", &pin8, 1, _alt );
}

void PwmOut::claim_pin( void )
{
    if ( -1 == _pin )
        return;

    take_channel();
}

void PwmOut::release_pin( void )
{
    _holds_pin  = false;
    pin_registry_forget( this );
}

void PwmOut::apply( void )
{
    if ( -1 == _pin )
        return;

    TPM_Type    *base   = s_tpm[ _tpm ];
    TpmState    &st     = s_state[ _tpm ];
    uint32_t    now     = (uint32_t)micros();

    // The smallest prescaler (highest resolution) for which the period
    // still fits the 16-bit counter. At most 0xFFFF ticks, so MOD stays
    // below 0xFFFF: a 100% pulse needs CnV > MOD, and CnV is 16 bits too
    uint8_t     prescale    = 0;
    uint32_t    ticks       = 0;
    for ( prescale = 0; prescale <= 7; prescale++ )
    {
        ticks = ticks_at( st.period_ticks, prescale );
        if ( ticks <= 0xFFFFu || prescale == 7 )
            break;
    }
    if ( ticks < 2 )
        ticks = 2;
    if ( ticks > 0xFFFFu )
        ticks = 0xFFFFu;

    // The prescaler can only change with the counter stopped. Stopped, the
    // registers take MOD and CnV at once, so nothing is left waiting
    uint32_t    cur_ps  = ( base->SC & TPM_SC_PS_MASK ) >> TPM_SC_PS_SHIFT;
    if ( cur_ps != prescale || !( base->SC & TPM_SC_CMOD_MASK ) )
    {
        TPM_StopTimer( base );
        base->SC        = ( base->SC & ~( TPM_SC_PS_MASK | TPM_SC_TOF_MASK ) ) | TPM_SC_PS( prescale );
        base->CNT       = 0u;
        st.waiting      = 0u;
    }

    set_mod( _tpm, ticks - 1u, now );

    for ( int ch = 0; ch < CHANNELS; ch++ )
    {
        PwmOut  *o  = st.owner[ ch ];

        if ( !o )
            continue;

        // A pulse as long as the period needs CnV = MOD + 1 to stay high
        uint32_t    cnv = ticks_at( o->_pulse_ticks, prescale );
        if ( o->_pulse_ticks >= st.period_ticks || cnv > ticks )
            cnv = ticks;

        set_cnv( _tpm, ch, (uint16_t)cnv, now );
    }

    if ( !( base->SC & TPM_SC_CMOD_MASK ) )
        TPM_StartTimer( base, kTPM_SystemClock );
}

void PwmOut::set_period( uint32_t ticks )
{
    if ( -1 == _pin )
        return;

    TpmState    &st = s_state[ _tpm ];

    if ( ticks < MIN_TICKS )
        ticks = MIN_TICKS;
    if ( ticks > MAX_TICKS )
        ticks = MAX_TICKS;

    st.period_ticks = ticks;

    for ( int ch = 0; ch < CHANNELS; ch++ )
    {
        if ( st.owner[ ch ] && st.owner[ ch ]->_pulse_ticks > st.period_ticks )
            st.owner[ ch ]->_pulse_ticks    = st.period_ticks;
    }

    apply();
}

void PwmOut::set_pulse( uint32_t ticks )
{
    if ( -1 == _pin )
        return;

    TpmState    &st = s_state[ _tpm ];

    _pulse_ticks    = ( ticks > st.period_ticks ) ? st.period_ticks : ticks;

    if ( st.owner[ _channel ] != this )
        take_channel();     // a pin sharing the channel had it: this one takes it back
    else
        apply();
}

void PwmOut::period( float seconds ) { set_period( seconds_to_ticks( seconds ) ); }
void PwmOut::period_ms( int ms )     { set_period( us_to_ticks( (int64_t)ms * 1000 ) ); }
void PwmOut::period_us( int us )     { set_period( us_to_ticks( us ) ); }

void PwmOut::frequency( uint32_t hz )
{
    if ( hz < 1u )
        hz = 1u;

    set_period( ( source_clock() + hz / 2u ) / hz );
}

void PwmOut::pulsewidth( float seconds ) { set_pulse( seconds_to_ticks( seconds ) ); }
void PwmOut::pulsewidth_ms( int ms )     { set_pulse( us_to_ticks( (int64_t)ms * 1000 ) ); }
void PwmOut::pulsewidth_us( int us )     { set_pulse( us_to_ticks( us ) ); }

void PwmOut::write( float duty )
{
    if ( -1 == _pin )
        return;

    if ( !( duty > 0.0f ) )     // NaN too
        duty = 0.0f;
    if ( duty > 1.0f )
        duty = 1.0f;

    set_pulse( (uint32_t)( duty * (float)s_state[ _tpm ].period_ticks + 0.5f ) );
}

void PwmOut::write_ratio( uint32_t num, uint32_t den )
{
    if ( -1 == _pin )
        return;

    uint32_t    period  = s_state[ _tpm ].period_ticks;

    if ( den == 0u )
        num = 0u;
    else if ( num > den )
        num = den;

    set_pulse( den ? (uint32_t)( ( (uint64_t)period * num + den / 2u ) / den ) : 0u );
}

float PwmOut::read( void )
{
    if ( -1 == _pin )
        return 0.0f;

    const TpmState &st = s_state[ _tpm ];

    return st.period_ticks ? ( (float)_pulse_ticks / (float)st.period_ticks ) : 0.0f;
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
