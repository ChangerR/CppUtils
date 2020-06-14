#ifndef __BOATSERILA__H
#define __BOATSERILA__H

#define MAX_SERIALBUFFER_SIZE 4096*4

enum BUADRATE {
	_B9600,_B19200,_B115200,
};

class Serial {
public:
	Serial(const char* com);
	virtual ~Serial();

	bool begin(BUADRATE b);
	void close();
	int write(const char* buf,int len);
	int read(char* buf,int len);

	int available();
	int readline(char* buf,int maxlen);
	int print(int n);
	int print(float f);
	int print(const char* s,...);
	int println(const char* s,...);

	bool touchForCDCReset();

	bool isAlive();
private:
	int read();
	void* _user;
	char _buffer[MAX_SERIALBUFFER_SIZE];
	int   _wpos;
	char  _port[32];
	bool _running;
	int _serial_fd;
};
#endif
