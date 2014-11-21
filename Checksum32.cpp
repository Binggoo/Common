#include "StdAfx.h"
#include "Checksum32.h"


CChecksum32::CChecksum32(void)
{
	reset();
}

CChecksum32::CChecksum32( const void* input, size_t length )
{
	reset();
	update(input,length);
}

CChecksum32::CChecksum32( const string& str )
{
	reset();
	update(str);
}

CChecksum32::CChecksum32( ifstream& in )
{
	reset();
	update(in);
}

CChecksum32::CChecksum32( const char *file )
{
	reset();
	update(file);
}


CChecksum32::~CChecksum32(void)
{
}

void CChecksum32::update( const void* input, size_t length )
{
	_update((const byte *)input,length);
}

void CChecksum32::update( const string& str )
{
	_update((const byte *)str.c_str(),str.length());
}

void CChecksum32::update( ifstream& in )
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

void CChecksum32::update( const char *file )
{
	ifstream in(file);
	update(in);
}

const byte* CChecksum32::digest()
{
	memcpy(_digest,&m_checksum,4);

	return _digest;
}

void CChecksum32::reset()
{
	m_checksum = 0;
	memset(_digest,0,4);
}

int CChecksum32::getHashLength()
{
	return 4;
}

void CChecksum32::_update( const byte *input,size_t length )
{
	for (size_t i = 0; i < length;i++)
	{
		m_checksum = (m_checksum << 31) | (m_checksum >> 1) + input[i];
	}
}

char * CChecksum32::getHashMetod()
{
	return "CHECKSUM32";
}
