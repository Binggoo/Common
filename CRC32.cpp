#include "StdAfx.h"
#include "CRC32.h"

using namespace std;

CCRC32::CCRC32(void)
{
	reset();
}

CCRC32::CCRC32( const void* input, size_t length ) : CHashMethod(input,length)
{
	reset();
	update(input,length);
}

CCRC32::CCRC32( const string& str ) : CHashMethod(str)
{
	reset();
	update(str);
}

CCRC32::CCRC32( ifstream& in ) : CHashMethod(in)
{
	reset();
	update(in);
}

CCRC32::CCRC32( const char *file ) : CHashMethod(file)
{
	reset();
	update(file);
}


CCRC32::~CCRC32(void)
{
}

void CCRC32::genCRC32table()
{
	int i;  
	int j;  
	unsigned int crc;  
	for(i = 0; i < 256; i++) 
	{  
		crc = i;  
		for(j = 0; j < 8; j++) 
		{
			if((crc & 1) == 1)  
				crc = (crc >> 1) ^ 0xEDB88320;  
			else  
				crc >>= 1;  
		}  
		m_crc32table[i] = crc;  
	}
}

void CCRC32::reset()
{
	m_crc32 = 0xFFFFFFFF;
	memset(_digest,0,4);
	genCRC32table();
}

void CCRC32::update( const void* input, size_t length )
{
	_update((const byte *)input,length);
}

void CCRC32::update( const string& str )
{
	_update((const byte *)str.c_str(),str.length());
}

void CCRC32::update( ifstream& in )
{
	if (!in) 
	{
		return;
	}

	std::streamsize length;
	char buffer[BUFFER_SIZE];
	while (!in.eof()) 
	{
		in.read(buffer, BUFFER_SIZE);
		length = in.gcount();
		if (length > 0) 
		{
			update(buffer, (size_t)length);
		}
	}
	in.close();
}

void CCRC32::update( const char *file )
{
	ifstream in(file);
	update(in);
}

const byte* CCRC32::digest()
{
	memcpy(_digest,&m_crc32,4);

	return _digest;
}

void CCRC32::_update( const byte *input,size_t length )
{
	for(size_t i = 0; i < length; i++)
	{
		m_crc32 = (m_crc32 >> 8) ^ m_crc32table[(m_crc32 ^ input[i]) & 0xFF]; 
	}
}

int CCRC32::getHashLength()
{
	return 4;
}

char * CCRC32::getHashMetod()
{
	return "CRC32";
}
