/**
 * @file    AnalogIn.cpp
 * @brief   Mbed-compatible AnalogIn implementation for r01lib (FRDM-MCXA153)
 *
 * Verified against the FRDM-MCXA153 SDK LPADC polling example
 * (examples/driver_examples/lpadc/polling) before writing this file:
 *  - referenceVoltageSource for VDDA on this board is
 *    `kLPADC_ReferenceVoltageAlt3` (not Alt1 — Alt1/Alt2/Alt3 are just
 *    numbered options; which one is VDDA is board-wiring dependent).
 *  - powerLevelMode: Alt1 is the *lowest* power/precision setting and
 *    Alt4 the highest (SDK doc order runs low->high), so Alt4 is used here.
 *  - `LPADC_DoAutoCalibration()` is required on this part, gated by
 *    FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ (both this feature and
 *    ...CTRL_CALOFS happen to be 1 on MCXA153, but CAL_REQ is the one the
 *    NXP example actually gates the calibration call with).
 *  - FSL_FEATURE_LPADC_FIFO_COUNT is 1 on this part, so `LPADC_GetConvResult()`
 *    takes no FIFO-index argument and FIFO reset is `LPADC_DoResetFIFO()`
 *    (no "0" suffix) — the 2-FIFO variants of these calls don't exist here.
 *
 * @author  Tedd OKANO
 * @license MIT
 */

#if defined( CPU_MCXA153VLH )

extern "C" {
#include "fsl_reset.h"
}

#include "AnalogIn.h"
#include "mcu.h"

uint8_t AnalogIn::_instance_count = 0;
bool    AnalogIn::_calibrated     = false;

namespace {

struct AnalogPinDescriptor {
    int         pin;             // io.h logical pin (A0..A3)
    PORT_Type  *port;
    uint32_t    port_pin;
    uint32_t    input_positive;  // hardware ADC0_An number
};

const AnalogPinDescriptor s_pins[] = {
    //  pin  port   pin#  ADC0_An
    { A0,  PORT1, 10u,  8u },
    { A1,  PORT1, 12u, 10u },
    { A2,  PORT1, 13u, 11u },
    { A3,  PORT2,  0u,  0u },
};

} // namespace

void AnalogIn::resolve_pin( int pin )
{
    for ( size_t i = 0; i < sizeof( s_pins ) / sizeof( s_pins[0] ); i++ )
    {
        if ( s_pins[ i ].pin == pin )
        {
            _input_positive = (uint8_t)s_pins[ i ].input_positive;
            _channel_id     = (uint8_t)( i + 1 );   // dedicated LPADC CMD1..CMD4
            _pin            = pin;

            port_pin_config_t cfg = {
                kPORT_PullDisable,
                kPORT_LowPullResistor,
                kPORT_FastSlewRate,
                kPORT_PassiveFilterDisable,
                kPORT_OpenDrainDisable,
                kPORT_LowDriveStrength,
                kPORT_NormalDriveStrength,
                kPORT_MuxAlt0,               // ADC0_An
                kPORT_InputBufferDisable,    // required for analog function
                kPORT_InputNormal,
                kPORT_UnlockRegister
            };
            PORT_SetPinConfig( s_pins[ i ].port, s_pins[ i ].port_pin, &cfg );
            return;
        }
    }

    _input_positive = 0;
    _channel_id      = 0;
    _pin             = -1;
}

void AnalogIn::_acquire_peripheral( void )
{
    if ( _instance_count == 0 )
    {
        CLOCK_EnableClock( kCLOCK_GateADC0 );
        RESET_ReleasePeripheralReset( kADC0_RST_SHIFT_RSTn );

        // ADCK (conversion clock) source — without this, LPADC_Init()
        // succeeds but no conversion/calibration cycle ever completes and
        // LPADC_DoAutoCalibration() spins forever polling GCC[0].RDY.
        CLOCK_SetClockDiv( kCLOCK_DivADC0, 1u );
        CLOCK_AttachClk( kFRO12M_to_ADC0 );

        lpadc_config_t cfg;
        LPADC_GetDefaultConfig( &cfg );
        cfg.enableAnalogPreliminary = true;
        cfg.referenceVoltageSource  = kLPADC_ReferenceVoltageAlt3;   // VDDA (FRDM-MCXA153)
        cfg.powerLevelMode          = kLPADC_PowerLevelAlt4;         // highest accuracy
        LPADC_Init( ADC0, &cfg );

#if defined( FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ ) && FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ
        if ( !_calibrated )
        {
            LPADC_DoAutoCalibration( ADC0 );
            _calibrated = true;
        }
#endif
    }

    _instance_count++;
}

void AnalogIn::_release_peripheral( void )
{
    _instance_count--;

    if ( _instance_count == 0 )
    {
        LPADC_Deinit( ADC0 );
        _calibrated = false;   // re-calibrate on the next acquire after a full deinit
    }
}

AnalogIn::AnalogIn( int pin )
    : Obj( true ), _channel_id( 0 ), _input_positive( 0 ), _pin( -1 )
{
    resolve_pin( pin );

    if ( -1 == _pin )
    {
        panic( "AnalogIn: unsupported analog pin" );
        return;
    }

    _acquire_peripheral();

    lpadc_conv_command_config_t cmd;
    LPADC_GetDefaultConvCommandConfig( &cmd );
    cmd.channelNumber            = _input_positive;
    cmd.conversionResolutionMode = kLPADC_ConversionResolutionStandard;  // 12-bit
    LPADC_SetConvCommandConfig( ADC0, _channel_id, &cmd );
}

AnalogIn::~AnalogIn()
{
    if ( -1 == _pin )
        return;

    _release_peripheral();
}

uint16_t AnalogIn::sample_raw( void )
{
    if ( -1 == _pin )
        return 0;

    lpadc_conv_trigger_config_t trig;
    LPADC_GetDefaultConvTriggerConfig( &trig );
    trig.targetCommandId       = _channel_id;
    trig.enableHardwareTrigger = false;
    LPADC_SetConvTriggerConfig( ADC0, 0u, &trig );

    LPADC_DoResetFIFO( ADC0 );
    LPADC_DoSoftwareTrigger( ADC0, 1u );   // trigger0 mask

    lpadc_conv_result_t result;
    while ( !LPADC_GetConvResult( ADC0, &result ) )
        ;

    return (uint16_t)( ( result.convValue >> 3u ) & 0x0FFFu );
}

uint16_t AnalogIn::read_u16( void )
{
    return (uint16_t)( sample_raw() << 4u );
}

float AnalogIn::read( void )
{
    return (float)read_u16() / 65535.0f;
}

AnalogIn::operator float()
{
    return read();
}

#endif // defined( CPU_MCXA153VLH )
