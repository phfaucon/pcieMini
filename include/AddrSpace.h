#pragma once
#include "AlphiDll.h"
#include "AlphiErrorCodes.h"

/** @brief Generic class for memory access
 *
 * The address space is conceptually an array but might not be accessible though a pointer.
 */
class AddrSpace
{
public:
	inline AddrSpace(){}

	/** @brief return the maximum index of the segment
	 * @retval Size in byte.
	 */
	virtual size_t		getLength()
      { return 0; }
	virtual const char *		getName()
      { return "uninitialized"; }
	virtual PCIeMini_status	writeU8(size_t offset, uint8_t val)
      { return 0; }
	virtual PCIeMini_status	writeU16(size_t offset, uint16_t val)
      { return 0; }
	virtual PCIeMini_status	writeU32(size_t offset, uint32_t val)
      { return 0; }
	virtual uint8_t		readU8(size_t offset)
      { return 0; }
	virtual uint16_t		readU16(size_t offset)
      { return 0; }
	virtual uint32_t		readU32(size_t offset)
      { return 0; }
	virtual const char *		toString()
      { return "uninitialized"; }
};

extern const char szHelpAddressSpace[];
void AddressSpace(char *szParm);
