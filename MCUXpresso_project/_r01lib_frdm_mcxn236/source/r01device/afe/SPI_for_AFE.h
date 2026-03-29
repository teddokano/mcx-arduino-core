/** NXP Analog Front End class library for MCX
 *
 *  @author  Tedd OKANO
 *
 *  Copyright: 2023 - 2024 Tedd OKANO
 *  Released under the MIT license
 */

#ifndef ARDUINO_SPI_FOR_AFE_H
#define ARDUINO_SPI_FOR_AFE_H

#include	"r01lib.h"
#include	<stdint.h>

class SPI_for_AFE
{
public:
	SPI_for_AFE( SPI& spi, bool dev_ad );
	virtual ~SPI_for_AFE();
	
	/** Send data
	 * 
	 * @param data pointer to data buffer
	 * @param size data size
	 */
	void txrx( uint8_t *data, int size );

	/** Register write, 8 bit
	 *
	 * @param reg register index
	 */
	void write_r16( uint16_t reg );

	/** Register write, 16 bit
	 *
	 * @param reg register index
	 * @param val data value
	 */
	void write_r16( uint16_t reg, uint16_t val );

	/** Register read, 16 bit
	 *
	 * @param reg register index
	 * @return data value
	 */
	uint16_t read_r16( uint16_t reg );

	/** Register write, 24 bit
	 *
	 * @param reg register index
	 * @param val data value
	 */
	void write_r24( uint16_t reg, uint32_t val );

	/** Register read, 24 bit
	 *
	 * @param reg register index
	 * @return data value
	 */
	int32_t read_r24( uint16_t reg );
	
	void burst( uint32_t *data, int length, int width = 3 );

private:

	//	functions to access AFE multibyte data access independent from endianess
	inline int32_t get_data16( uint8_t *vp )
	{
		return ((uint16_t)(*(vp + 0)) << 8) | *(vp + 1);
	}
	
	inline int32_t get_data24( uint8_t *vp )
	{
		int32_t	r0	= *(vp + 0);
		int32_t	r1	= *(vp + 1);
		int32_t	r2	= *(vp + 2);
		int32_t	r	= ( (r0 << 24) | (r1 << 16) | (r2 << 8) );

		return r >> 8;
	}

	static constexpr int	command_length	= 2;
	SPI& 		_spi;
	const bool	dev_ad;
};

#endif //	ARDUINO_SPI_FOR_AFE_H
