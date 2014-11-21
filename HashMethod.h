#ifndef __HASH_METHOD_H__
#define __HASH_METHOD_H__

#include <string>
#include <fstream>

/* Type define */
typedef unsigned char byte;
typedef unsigned int uint32;

using std::string;
using std::ifstream;

class CHashMethod
{
public:
	CHashMethod(void);
	CHashMethod(const void* input, size_t length);
	CHashMethod(const string& str);
	CHashMethod(ifstream& in);
	CHashMethod(const char *file);
	~CHashMethod(void);

	virtual void reset() = 0;
	virtual void update(const void* input, size_t length) = 0;
	virtual void update(const string& str) = 0;
	virtual void update(ifstream& in) = 0;
	virtual void update(const char *file) = 0;
	virtual const byte* digest() = 0;
	virtual int getHashLength() = 0;
	virtual char *getHashMetod() = 0;

protected:
	enum { BUFFER_SIZE = 1024 };
};

#endif