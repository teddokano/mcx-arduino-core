/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"led/GradationControl.h"

GradationControl::GradationControl( PCA995x* dev_ptr, int group_num, uint32_t bitmap ) : 
	devp( dev_ptr ), group( group_num )
{
	gradation_group_assign( bitmap );
	gradation_channel_enable( bitmap );
}

GradationControl::~GradationControl()
{
}

float GradationControl::set_gradation( float max_iref, float time, bool up, bool down, float on, float off, bool hold_on, bool hold_off )
{
	int		iref;
	float	step_duration;
	float	cycle_time;
	int		cycle_time_i	= 0;
	int		multi_fctr		= 1;
	int		iref_inc		= 1;
	int		on_i			= 7;
	int		off_i			= 7;
	float	ramp_time;
	uint8_t	reg[ 4 ];
	
	iref	= (int)(max_iref * 255.0);
	time	*= 1000.0;
	
	if ( 0.0 < time ) {
		step_duration	= time / iref;

		if ( 32 < step_duration ) {
			cycle_time		= 8.0;
			cycle_time_i	= 1;
		}
		else {
			cycle_time		= 0.5;
			cycle_time_i	= 0;
		}

		multi_fctr	= (int)(step_duration / cycle_time);
		multi_fctr	= (multi_fctr <  1) ?  1 : multi_fctr;
		multi_fctr	= (64 < multi_fctr) ? 64 : multi_fctr;
		
		if ( multi_fctr == 1 )
			iref_inc	= (int)( iref / (time / cycle_time) );
	}
	else {
		cycle_time	= 0;
	}
	
	for ( int i = 0; i < 7; i++ ) {
		if ( on < HOLDTIME[ i + 1 ] ) {
			on		= HOLDTIME[ i ];
			on_i	= i;
			break;
		}
	}		

	for ( int i = 0; i < 7; i++ ) {
		if ( off < HOLDTIME[ i + 1 ] ) {
			off		= HOLDTIME[ i ];
			off_i	= i;
			break;
		}
	}

	reg[ 0 ]	= (up << 7) | (down << 6) |  (iref_inc - 1);					// for RAMP_RATE_GRPn
	reg[ 1 ]	= ( cycle_time_i << 6 ) | (multi_fctr - 1);						// for STEP_TIME_GRPn
	reg[ 2 ]	= (hold_on << 7) | (hold_off << 6) | ( on_i << 3) | off_i;		// for HOLD_CNTL_GRPn
	reg[ 3 ]	= iref;															// for IREF_GRPn
	
	devp->reg_access( devp->arp[ SETTING ] + group * 4, reg, sizeof( reg ) );
	
	ramp_time	= ((multi_fctr * cycle_time) * (iref / iref_inc)) / 1000;
	
	cycle_time	 = on + off;
	cycle_time	+= up   ? ramp_time : 0;
	cycle_time	+= down ? ramp_time : 0;

	cycle	= cycle_time;
	
	return cycle;
}

void GradationControl::gradation_channel_enable( uint32_t bitmap )
{
	const int	n_reg	= devp->n_channel / 8;
	uint8_t		reg[ n_reg ];

	devp->reg_access_r( devp->arp[ MODE ], reg, sizeof( reg ) );
	
	for ( int i = 0; i < n_reg; i++ ) {
		reg[ i ]	|= (uint8_t)(bitmap >> (i * 8));
	}

	devp->reg_access( devp->arp[ MODE ], reg, sizeof( reg ) );
}

void GradationControl::gradation_group_assign( uint32_t bitmap )
{
	const int	grp_shift	= (devp->arp[ N_GROUP ] == 4) ? 2 : 4;
	const int	n_reg		= devp->n_channel / (8 / grp_shift);
	uint8_t		bits		= (grp_shift == 2) ? 0x3 : 0x0F;
	uint8_t		reg[ n_reg ];
	int			r_index;
	int			s_index;
	
	devp->reg_access_r( devp->arp[ GROUP ], reg, sizeof( reg ) );
	
	for ( int i = 0; i < devp->n_channel; i++ ) {
		if ( (bitmap >> i) & 0x1 ) {
			r_index	= i / (8 / grp_shift);
			s_index	= (i * grp_shift) % 8;
			
			reg[ r_index ]	 = ~(bits << s_index) & reg[ r_index ];
			reg[ r_index ]	|= group << s_index;
		}
	}

	devp->reg_access( devp->arp[ GROUP ], reg, sizeof( reg ) );
}

void GradationControl::add_channel( uint8_t ch )
{
	const int	grp_shift	= (devp->arp[ N_GROUP ] == 4) ? 2 : 4;
	uint8_t		bits		= (grp_shift == 2) ? 0x3 : 0x0F;
	uint8_t		v;
	int			r_index;
	int			s_index;
	
	
	r_index	= ch / (8 / grp_shift);
	s_index	= (ch * grp_shift) % 8;

	v	= devp->reg_access( devp->arp[ GROUP ] + r_index );
	
	v	 = ~(bits << s_index) & v;
	v	|= group << s_index;

	devp->reg_access( devp->arp[ GROUP ] + r_index, v );
	
	const int	n_reg	= ch / 8;
	
	v	= devp->reg_access( devp->arp[ MODE ] + n_reg );
	
	v	|= (uint8_t)(0x1 << (ch % 8));

	devp->reg_access( devp->arp[ MODE ] + n_reg, v );
}


void GradationControl::control( int ctrl )
{
	int			r_index;
	int			s_index;
	uint8_t		reg;
	
	r_index	= group / 4;
	s_index	= (group * 2) % 8 + (r_index ? 4 : 0);
	
	
	reg	 = devp->reg_access( devp->arp[ CNTL ] + r_index );
	reg	&= ~(0x03 << s_index);
	reg	|= ctrl << s_index;

	devp->reg_access( devp->arp[ CNTL ] + r_index, reg );
}
void GradationControl::start( bool continuous )
{
	control( ONE_SHOT | continuous );
}

void GradationControl::stop( void )
{
	control( GRADATION_STOP );
}


const float GradationControl::HOLDTIME[]	= { 0, 0.25, 0.50, 0.75, 1, 2, 4, 6, };

