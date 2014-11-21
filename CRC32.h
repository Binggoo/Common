#ifndef __CRC32_H__
#define __CRC32_H__
#include "HashMethod.h"

class CCRC32 : public CHashMethod
{
public:
	CCRC32(void);
	CCRC32(const void* input, size_t length);
	CCRC32(const string& str);
	CCRC32(ifstream& in);
	CCRC32(const char *file);
	
	~CCRC32(void);

	void update(const void* input, size_t length);
	void update(const string& str);
	void update(ifstream& in);
	void update(const char *file);
	const byte* digest();
	void reset();

	int getHashLength();
	void genCRC32table();
	char *getHashMetod();

private:
	uint32 m_crc32;
	uint32 m_crc32table[256];
	byte _digest[4];

	void _update(const byte *input,size_t length);
};

#endif