/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_SPI_H
#define R01LIB_SPI_H

extern "C" {
#ifdef	CPU_MCXC444VLH
#include "fsl_spi.h"
#else
#include "fsl_lpspi.h"
#endif
}

#include	"io.h"

/** default SCLK frequency (Hz) used by frequency() when no argument is given */
#define	SPI_FREQ		1'000'000UL


/** SPI class
 *
 *  @class SPI
 *
 *	A class for SPI bus operations
 */

class SPI : public Obj
{
public:
	
	/** Create a SPI instance with specified pins
	 *
	 * @param mosi (optional) pin number to connect MOSI
	 * @param miso (optional) pin number to connect MISO
	 * @param sclk (optional) pin number to connect SCLK
	 * @param cs   (optional) pin number to connect CS
	 */
	SPI( int mosi = D11, int miso = D12, int sclk = D13, int cs = D10 );
	
	/** Destructor to free SPI resource
	 */
	virtual ~SPI();

	/** Frequency settings
	 *
	 * @param frequency (optional) SCLK frequency in Hz
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

	/** Bit order setting
	 *
	 * @param order 0 = LSB first, non-zero = MSB first (matches Arduino's
	 *              LSBFIRST=0/MSBFIRST=1 convention)
	 */
	virtual void	bit_order( uint8_t order );

	/** SPI peripheral's input clock, in Hz -- the reference frequency() is
	 *  divided down from.
	 */
	uint32_t	clock_freq( void ) const { return master_clk_freq; }

	/** Data transfer on SPI
	 *  
	 * @param wp data to write
	 * @param rp data buffer for read
	 * @param length transfer length
	 */	
	virtual status_t		write( uint8_t *wp, uint8_t *rp, int length );

	/** Fast scalar single-byte transfer.
	 *
	 *  Bypasses write()'s generic blocking-transfer per-call overhead
	 *  (disable/flush-FIFO/clear-status-flags/re-enable), which dominates
	 *  cost for a single 8-bit frame -- this exists for callers that move
	 *  data one byte at a time in a tight loop (e.g. the standard Arduino
	 *  SD library's spiRec()/spiSend()).
	 *
	 * @param out byte to transmit
	 * @return byte received
	 */
	virtual uint8_t			transfer_byte( uint8_t out );

	/** Manual CS control setting
	 *
	 *  Switches the CS pin's mux between plain GPIO (so the sketch can
	 *  drive it directly via the returned DigitalOut*) and the LPSPI
	 *  peripheral's own hardware PCS function.
	 *
	 * @param flag manual setting = true, auto (hardware PCS) control = false
	 * @return pointer to the CS pin's DigitalOut, for manual drive when flag is true
	 */
	virtual DigitalOut* cs_manual_control( bool flag );

	/** variable for reporting last state */
	status_t				last_status;

protected:
	DigitalOut				chip_select;
	bool					manual_cs_control;
private:
#ifdef	CPU_MCXC444VLH
	spi_master_config_t		masterConfig;
	SPI_Type				*unit_base;
#else
	lpspi_master_config_t	masterConfig;
	LPSPI_Type				*unit_base;
#endif
	
	uint32_t				master_clk_freq;
	uint32_t				master_pcs_4_xfer;
};

#endif // R01LIB_SPI_H
