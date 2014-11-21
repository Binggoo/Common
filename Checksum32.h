#ifndef __CHECKSUM32_H__
#define __CHECKSUM32_H__
#include "HashMethod.h"

class CChecksum32 : public CHashMethod
{
public:
	CChecksum32(void);
	CChecksum32(const void* input, size_t length);
	CChecksum32(const string& str);
	CChecksum32(ifstream& in);
	CChecksum32(const char *file);

	~CChecksum32(void);

	void update(const void* input, size_t length);
	void update(const string& str);
	void update(ifstream& in);
	void update(const char *file);
	const byte* digest();
	void reset();

	int getHashLength();
	char *getHashMetod();

private:
	uint32 m_checksum;
	byte _digest[4];

	void _update(const byte *input,size_t length);
};

#endif