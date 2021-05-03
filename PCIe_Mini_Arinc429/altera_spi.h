#ifndef __ALT_AVALON_SPI_H__
#define __ALT_AVALON_SPI_H__
#include <stddef.h>

//#include "alt_types.h"

#ifdef __cplusplus
#define BUS_WIDTH 1
extern "C"
{
#endif /* __cplusplus */
#define ALTERA_AVALON_SPI_RXDATA_REG                  0
#define IOADDR_ALTERA_AVALON_SPI_RXDATA(base)         (base)
#define IORD_ALTERA_AVALON_SPI_RXDATA(base)           (base[0]) 
#define IOWR_ALTERA_AVALON_SPI_RXDATA(base, data)     (base[0] = data)

#define ALTERA_AVALON_SPI_TXDATA_REG                  1
#define IOADDR_ALTERA_AVALON_SPI_TXDATA(base)         (base + 1*BUS_WIDTH)
#define IORD_ALTERA_AVALON_SPI_TXDATA(base)           (base[1*BUS_WIDTH]) 
#define IOWR_ALTERA_AVALON_SPI_TXDATA(base, data)     (base[1*BUS_WIDTH] = data)

#define ALTERA_AVALON_SPI_STATUS_REG                  2
#define IOADDR_ALTERA_AVALON_SPI_STATUS(base)         (base + 2*BUS_WIDTH)
#define IORD_ALTERA_AVALON_SPI_STATUS(base)           (base[2*BUS_WIDTH]) 
#define IOWR_ALTERA_AVALON_SPI_STATUS(base, data)     (base[2*BUS_WIDTH] = data)

#define ALTERA_AVALON_SPI_STATUS_ROE_MSK              (0x8)
#define ALTERA_AVALON_SPI_STATUS_ROE_OFST             (3)
#define ALTERA_AVALON_SPI_STATUS_TOE_MSK              (0x10)
#define ALTERA_AVALON_SPI_STATUS_TOE_OFST             (4)
#define ALTERA_AVALON_SPI_STATUS_TMT_MSK              (0x20)
#define ALTERA_AVALON_SPI_STATUS_TMT_OFST             (5)
#define ALTERA_AVALON_SPI_STATUS_TRDY_MSK             (0x40)
#define ALTERA_AVALON_SPI_STATUS_TRDY_OFST            (6)
#define ALTERA_AVALON_SPI_STATUS_RRDY_MSK             (0x80)
#define ALTERA_AVALON_SPI_STATUS_RRDY_OFST            (7)
#define ALTERA_AVALON_SPI_STATUS_E_MSK                (0x100)
#define ALTERA_AVALON_SPI_STATUS_E_OFST               (8)

#define ALTERA_AVALON_SPI_CONTROL_REG                 3
#define IOADDR_ALTERA_AVALON_SPI_CONTROL(base)        (base + 3*BUS_WIDTH)
#define IORD_ALTERA_AVALON_SPI_CONTROL(base)          (base[3*BUS_WIDTH]) 
#define IOWR_ALTERA_AVALON_SPI_CONTROL(base, data)    (base[3*BUS_WIDTH] = data)

#define ALTERA_AVALON_SPI_CONTROL_IROE_MSK            (0x8)
#define ALTERA_AVALON_SPI_CONTROL_IROE_OFST           (3)
#define ALTERA_AVALON_SPI_CONTROL_ITOE_MSK            (0x10)
#define ALTERA_AVALON_SPI_CONTROL_ITOE_OFST           (4)
#define ALTERA_AVALON_SPI_CONTROL_ITRDY_MSK           (0x40)
#define ALTERA_AVALON_SPI_CONTROL_ITRDY_OFS           (6)
#define ALTERA_AVALON_SPI_CONTROL_IRRDY_MSK           (0x80)
#define ALTERA_AVALON_SPI_CONTROL_IRRDY_OFS           (7)
#define ALTERA_AVALON_SPI_CONTROL_IE_MSK              (0x100)
#define ALTERA_AVALON_SPI_CONTROL_IE_OFST             (8)
#define ALTERA_AVALON_SPI_CONTROL_SSO_MSK             (0x400)
#define ALTERA_AVALON_SPI_CONTROL_SSO_OFST            (10)

#define ALTERA_AVALON_SPI_SLAVE_SEL_REG               5
#define IOADDR_ALTERA_AVALON_SPI_SLAVE_SEL(base)      (base + 5*BUS_WIDTH)
#define IORD_ALTERA_AVALON_SPI_SLAVE_SEL(base)        (base[5*BUS_WIDTH])
#define IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(base, data)  (base[5*BUS_WIDTH] = data)
	/*
	* Use this function to perform one SPI access on your target.  'base' should
	* be the base address of your SPI peripheral, while 'slave' indicates which
	* bit in the slave select register should be set.
	*/

	/* If you need to make multiple accesses to the same slave then you should
	* set the merge bit in the flags for all of them except the first.
	*/
#define ALT_AVALON_SPI_COMMAND_MERGE (0x01)

	/*
	* If you need the slave select line to be toggled between words then you
	* should set the toggle bit in the flag.
	*/
#define ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N (0x02)


	int alt_avalon_spi_command(UINT32 *base, UINT32 slave,
		UINT32 write_length, UINT8 * write_data,
		UINT32 read_length, UINT8 * read_data,
		UINT32 flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_AVALON_SPI_H__ */
