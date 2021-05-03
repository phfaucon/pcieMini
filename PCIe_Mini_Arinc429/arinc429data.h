#ifndef _ARINC_429_DATA_H
#define _ARINC_429_DATA_H

#define BYTE_SWAPPING
#ifndef BYTE_SWAPPING
class Arinc429word {
public:
	UINT32 arincWord;

	inline UINT8 getLabel() 
	{
		return (UINT8)(arincWord & 0xff);
	}

	inline void setLabel(UINT8 label) 
	{
		arincWord = (arincWord & 0xffffff00) | label;
	}

	inline UINT8 getParity()
	{
		return (arincWord & 0x80000000) != 0;
	}

	inline UINT32 getData()
	{
		return (arincWord >> 10) & 0x7ffff;
	}

	inline void setData(UINT32 data)
	{
		arincWord = (arincWord & ~0x1ffffc00) | (data << 10);
	}
};
#else
// swap the bytes during the instantiation
class Arinc429word {
public:
	inline Arinc429word(UINT32 wd)
	{
		arincWord = (wd & 0xff) << 24
			| (wd & 0xff00) << 8
			| (wd & 0xff0000) >> 8
			| (wd & 0xff000000) >> 24;
	}

	inline Arinc429word()
	{
		arincWord = 0;
	}

	inline UINT8 getLabel()
	{
		return (UINT8)(arincWord & 0xff);
	}

	inline void setLabel(UINT8 label)
	{
		arincWord = (arincWord & 0xffffff00) | label;
	}

	inline UINT8 getParity()
	{
		return (arincWord & 0x80000000) != 0;
	}

	inline UINT32 getData()
	{
		return (arincWord >> 10) & 0x7ffff;
	}

	inline void setData(UINT32 data)
	{
		arincWord = (arincWord & ~0x1ffffc00) | (data << 10);
	}
private:
	UINT32 arincWord;
};
#endif
#endif
