/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Original implementation of the official Servo library's interface on
 *  FlexPWM/TPM, and on a timer interrupt for the pins without PWM; none of
 *  its source (LGPL 2.1) is copied.
 */

#include "Servo.h"

extern "C" {
#if defined( CPU_MCXC444VLH )
#include	"fsl_tpm.h"
#else
#include	"fsl_ctimer.h"
#endif
#include	"fsl_clock.h"
}

//	arduino_io.cpp: the raw pin value for an Arduino pin number, -1 for none
int	arduino_pin_raw( int pin_num );

static uint8_t	servo_count	= 0;

//	The pins without PWM: one timer interrupt raises each attached pin in
//	turn, by index, and lowers it after its pulse width, then waits for the
//	rest of the 20ms. The timer counts freely and each interrupt sets the
//	next match from the last one, not from when it ran, so a late
//	interrupt shifts both ends of a pulse alike.
namespace {

struct Channel
{
	volatile uint32_t	*set_reg	= nullptr;	// the pin's GPIO PSOR
	volatile uint32_t	*clear_reg	= nullptr;	// and PCOR
	uint32_t			mask		= 0;
	volatile uint32_t	ticks		= 0;		// pulse width
	volatile bool		active		= false;
};

Channel		channels[ MAX_SERVOS ];
int			current		= -1;		// the channel whose pulse is on, -1 between frames
uint32_t	next_at		= 0;		// the match set last
uint32_t	frame_start	= 0;
bool		ready		= false;
bool		running		= false;
uint32_t	clock_khz	= 0;
uint32_t	margin_ticks	= 0;	// 10us: how far ahead a match is set at the least
uint32_t	refresh_ticks	= 0;	// 20ms

#if defined( CPU_MCXC444VLH )

#define	SERVO_IRQn			TPM2_IRQn
#define	SERVO_IRQHandler	TPM2_IRQHandler

constexpr uint32_t	COUNT_MASK	= 0xFFFFu;	// TPM2 counts 16 bits

void hw_init( void )
{
	//	IRC48M, which PwmOut picks for TPM0 and TPM1 too: the choice is
	//	one for all three. Divided by 32 to 1.5MHz, for 43.7ms a turn
	CLOCK_SetTpmClock( 1u );

	tpm_config_t	cfg;

	TPM_GetDefaultConfig( &cfg );
	cfg.prescale	= kTPM_Prescale_Divide_32;
	TPM_Init( TPM2, &cfg );

	clock_khz	= CLOCK_GetFreq( kCLOCK_McgIrc48MClk ) / 32u / 1000u;

	TPM2->MOD						= COUNT_MASK;
	TPM2->CONTROLS[ 0 ].CnSC		= TPM_CnSC_MSA_MASK | TPM_CnSC_CHIE_MASK;	// software compare: no pin
	TPM_StartTimer( TPM2, kTPM_SystemClock );
}

uint32_t hw_now( void )
{
	return	TPM2->CNT & COUNT_MASK;
}

void hw_set_match( uint32_t t )
{
	TPM2->CONTROLS[ 0 ].CnV	= t;	// in output compare, on the next count after the write
}

void hw_clear_flag( void )
{
	TPM2->STATUS	= TPM_STATUS_CH0F_MASK;
}

#else

#if defined( CPU_MCXN947VDF )
#define	SERVO_CTIMER		CTIMER3		// CTIMER2 is MCXEthernet's
#define	SERVO_CTIMER_INDEX	3u
#define	SERVO_IRQn			CTIMER3_IRQn
#define	SERVO_IRQHandler	CTIMER3_IRQHandler
#define	SERVO_CLOCK_ATTACH	kFRO_HF_to_CTIMER3
#elif defined( CPU_MCXA153VLH ) || defined( CPU_MCXA156VLL ) || defined( CPU_MCXN236VDF )
#define	SERVO_CTIMER		CTIMER2
#define	SERVO_CTIMER_INDEX	2u
#define	SERVO_IRQn			CTIMER2_IRQn
#define	SERVO_IRQHandler	CTIMER2_IRQHandler
#define	SERVO_CLOCK_ATTACH	kFRO_HF_to_CTIMER2
#else
#error "Servo: pick the timer for pins without PWM on this chip"
#endif

constexpr uint32_t	COUNT_MASK	= 0xFFFFFFFFu;

void set_divider( uint32_t div )
{
#if defined( CPU_MCXN947VDF )
	CLOCK_SetClkDiv( kCLOCK_DivCtimer3Clk, div );
#elif defined( CPU_MCXN236VDF )
	CLOCK_SetClkDiv( kCLOCK_DivCtimer2Clk, div );
#elif defined( CPU_MCXA153VLH ) || defined( CPU_MCXA156VLL )
	CLOCK_SetClockDiv( kCLOCK_DivCTIMER2, div );
#else
#error "Servo: set the timer's clock divider for this chip"
#endif
}

void hw_init( void )
{
	//	As TimerOne's CTIMER1: FRO_HF, which the core clock comes from too,
	//	divided to 12MHz
	CLOCK_AttachClk( SERVO_CLOCK_ATTACH );
	set_divider( 1u );
	set_divider( CLOCK_GetCTimerClkFreq( SERVO_CTIMER_INDEX ) / 12000000u );

	ctimer_config_t	cfg;

	CTIMER_GetDefaultConfig( &cfg );
	CTIMER_Init( SERVO_CTIMER, &cfg );

	clock_khz	= CLOCK_GetCTimerClkFreq( SERVO_CTIMER_INDEX ) / 1000u;

	ctimer_match_config_t	match	= {};
	match.matchValue			= 0;
	match.enableCounterReset	= false;	// counts freely
	match.enableInterrupt		= true;
	match.outControl			= kCTIMER_Output_NoAction;
	CTIMER_SetupMatch( SERVO_CTIMER, kCTIMER_Match_0, &match );

	NVIC_DisableIRQ( SERVO_IRQn );	// CTIMER_SetupMatch() turned it on
	CTIMER_StartTimer( SERVO_CTIMER );
}

uint32_t hw_now( void )
{
	return	SERVO_CTIMER->TC;
}

void hw_set_match( uint32_t t )
{
	SERVO_CTIMER->MR[ kCTIMER_Match_0 ]	= t;
}

void hw_clear_flag( void )
{
	CTIMER_ClearStatusFlags( SERVO_CTIMER, kCTIMER_Match0Flag );
}

#endif

constexpr uint32_t	HALF_TURN	= COUNT_MASK / 2u + 1u;

uint32_t us_to_ticks( uint32_t us )
{
	return	us * clock_khz / 1000u;
}

//	Whether a comes after b, on a counter that wraps
bool after( uint32_t a, uint32_t b )
{
	uint32_t	d	= ( a - b ) & COUNT_MASK;

	return	d != 0 && d < HALF_TURN;
}

//	The conversions done once, so that the interrupt doesn't divide (in
//	software on the Cortex-M0+)
void init_ticks( void )
{
	margin_ticks	= us_to_ticks( 10u );
	refresh_ticks	= us_to_ticks( REFRESH_INTERVAL );
}

void schedule( uint32_t t )
{
	t	&= COUNT_MASK;
	hw_set_match( t );

	//	A match already passed, or too close to set safely, would wait for
	//	the counter to come round again (6 minutes on CTIMER): run late
	//	instead. The flag, if any, is that passed match's
	if ( !after( t, ( hw_now() + margin_ticks / 2u ) & COUNT_MASK ) )
	{
		t	= ( hw_now() + margin_ticks ) & COUNT_MASK;
		hw_set_match( t );
		hw_clear_flag();
	}

	next_at	= t;
}

void service( void )
{
	uint32_t	t	= next_at;

	if ( current >= 0 && channels[ current ].clear_reg )
		*channels[ current ].clear_reg	= channels[ current ].mask;

	while ( ++current < MAX_SERVOS && !channels[ current ].active )
		;

	if ( current < MAX_SERVOS )
	{
		Channel	&c	= channels[ current ];

		*c.set_reg	= c.mask;
		t			+= c.ticks;
	}
	else
	{
		current	= -1;

		bool	any	= false;

		for ( const Channel &c : channels )
			any	= any || c.active;

		if ( !any )
		{
			NVIC_DisableIRQ( SERVO_IRQn );
			running	= false;
			return;
		}

		//	The next frame 20ms after this one began, or at once if the
		//	pulses took longer
		uint32_t	frame_end	= frame_start + refresh_ticks;
		uint32_t	soonest		= t + margin_ticks;

		t			= after( frame_end, soonest ) ? frame_end : soonest;
		frame_start	= t;
	}

	schedule( t );
}

void start( void )
{
	if ( running )
		return;

	current	= -1;
	schedule( hw_now() + margin_ticks );
	frame_start	= next_at;

	hw_clear_flag();
	NVIC_ClearPendingIRQ( SERVO_IRQn );
	NVIC_EnableIRQ( SERVO_IRQn );
	running	= true;
}

uint32_t irq_save( void )
{
	uint32_t	primask	= __get_PRIMASK();

	__disable_irq();
	return	primask;
}

void irq_restore( uint32_t primask )
{
	__set_PRIMASK( primask );
}

}	// namespace

extern "C" void SERVO_IRQHandler( void )
{
	hw_clear_flag();
	service();
}

Servo::Servo()
{
	index	= servo_count < MAX_SERVOS ? servo_count++ : INVALID_SERVO;
}

uint8_t Servo::attach( int pin )
{
	return	attach( pin, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH );
}

uint8_t Servo::attach( int pin, int min, int max )
{
	if ( index == INVALID_SERVO )
		return	INVALID_SERVO;

	//	The official library takes any pin, so sketches don't check what
	//	attach() returns: failing quietly here would leave the servo still
	//	with no hint why. Stop and say so, as analogWriteFrequency() does.
	if ( arduino_pin_raw( pin ) < 0 )
		panic( "Servo: no such pin" );

	PwmOut	*p	= analog_pwm_out( pin );

	if ( attached() && !( p && p == pwm ) )
		detach();	// moving to another pin: stop the old one's pulses

	this->pin	= pin;
	min_us		= min;
	max_us		= max;

	if ( p )
	{
		pwm	= p;
		pwm->period_us( REFRESH_INTERVAL );
		writeMicroseconds( pulse_us );	// clamps a width written before to the new range

		return	index;
	}

	pinMode( pin, OUTPUT );
	digitalWrite( pin, LOW );

	GPIO_Type	*port	= digitalPinToPort( pin );

	if ( port == nullptr )
		panic( "Servo: no such pin" );

	if ( !ready )
	{
		hw_init();
		init_ticks();
		ready	= true;
	}

	Channel		&c			= channels[ index ];
	uint32_t	primask		= irq_save();

	c.set_reg	= &port->PSOR;
	c.clear_reg	= &port->PCOR;
	c.mask		= digitalPinToBitMask( pin );
	irq_restore( primask );

	on_timer	= true;
	writeMicroseconds( pulse_us );

	primask		= irq_save();
	c.active	= true;
	start();
	irq_restore( primask );

	return	index;
}

void Servo::detach()
{
	if ( pwm )
		pwm->pulsewidth_us( 0 );

	if ( on_timer )
	{
		Channel		&c			= channels[ index ];
		uint32_t	primask		= irq_save();

		c.active		= false;
		*c.clear_reg	= c.mask;	// cuts a pulse that is on; the timer stops after the frame if none is left
		irq_restore( primask );
	}

	pwm			= nullptr;
	on_timer	= false;
	pin			= -1;
}

void Servo::write( int value )
{
	if ( value < MIN_PULSE_WIDTH )
	{
		value	= constrain( value, 0, 180 );
		value	= map( value, 0, 180, min_us, max_us );
	}

	writeMicroseconds( value );
}

void Servo::writeMicroseconds( int value )
{
	if ( index == INVALID_SERVO )
		return;

	pulse_us	= constrain( value, min_us, max_us );
	send();
}

void Servo::send()
{
	if ( pwm )
		pwm->pulsewidth_us( pulse_us );
	else if ( on_timer )
		channels[ index ].ticks	= us_to_ticks( (uint32_t)constrain( pulse_us, 0, REFRESH_INTERVAL ) );	// takes effect from the next pulse
}

int Servo::read()
{
	// The +1 makes a width written from an angle read back as that angle:
	// map() rounds down both ways
	return	map( readMicroseconds() + 1, min_us, max_us, 0, 180 );
}

int Servo::readMicroseconds()
{
	return	index == INVALID_SERVO ? 0 : pulse_us;
}

bool Servo::attached()
{
	return	pwm != nullptr || on_timer;
}
