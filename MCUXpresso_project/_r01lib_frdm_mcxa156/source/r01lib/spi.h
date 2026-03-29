/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef R01LIB_SPI_H
#define R01LIB_SPI_H

extern "C" {
#include "fsl_lpspi.h"
}

#include	"spi.h"
#include	"io.h"

#define	SPI_FREQ		1000000UL

/** SPI class
 *	
 *  @class SPI
 *
 *	A class for demonstrating SPI bus
 */

class SPI : public Obj
{
public:
	
	/** Create a SPI instance with specified pins
	 *
	 * @param mosi (option) pin number to connect MOSI
	 * @param miso (option) pin number to connect MISO
	 * @param sclk (option) pin number to connect SCLK
	 * @param cs (option) pin number to connect CS
	 */
	SPI( int mosi = D11, int miso = D12, int sclk = D13, int cs = D10 );
	
	/** Destractor to freeing SPI resource
	 */
	virtual ~SPI();

	/** Frequency settings
	 * 
	 * @param frequency (option) define default SCLK frequency
	 */
	virtual void	frequency( uint32_t frequency = SPI_FREQ );

	/** mode setting
	 *	SPI bus mode setting
	 *	mode 0 = CPOL:0, CPHA:0
	 *	mode 1 = CPOL:0, CPHA:1
	 *	mode 2 = CPOL:1, CPHA:0
	 *	mode 3 = CPOL:1, CPHA:1
	 *  
	 * @param mode selecting mode 0~3
	 */
	virtual void	mode( uint8_t mode = 0 );

	/** Data transfer on SPI
	 *  
	 * @param wp data to write
	 * @param rp data buffer for read
	 * @param length transfer length
	 */	
	virtual status_t		write( uint8_t *wp, uint8_t *rp, int length );

	/** variable for reporting last state */
	status_t				last_status;

private:
	lpspi_master_config_t	masterConfig;
	LPSPI_Type				*unit_base;
	uint32_t				master_clk_freq;
	uint32_t				master_pcs_4_xfer;
};

#endif // R01LIB_SPI_H
