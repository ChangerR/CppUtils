#ifndef __CPPUTILS_BUFFEREDSTREAM_
#define __CPPUTILS_BUFFEREDSTREAM_

#include "list.h"

typedef unsigned char u8;

class BufferedStream {
public:
	BufferedStream(int size_count,int cnt = 16);
	
	virtual ~BufferedStream();
	
public:
	
	void lock();
	
	void unlock();

	int read(u8* buf,int len);
	
	int readline(u8* buf,int len);
	
	int skip(int len);
	
	int write(const u8* wbuf,int len,bool blocked = false);
	
	int left();

	void alloc_new_page(int cnt);
	
	void setEOF(bool v);
	
	bool isEOF();
	
	int size();
	
	void clear();
	
private:
	int _page_size;
	list<u8*>* _free_page;
	list<u8*>* _p_arr;
	int _read_pos;
	int _write_pos;
	u8* _readpage_addr;
	u8* _writepage_addr;
	CRITICAL_SECTION _write_mutex;
	bool _isEOF;
};

#endif