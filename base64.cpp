#include <stdio.h>
#include <string.h>

char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char dbase64[] = {
	62,0,0,0,63,52,53,54,55,56,57,58,59,60,61,0,0,
	0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,
	13,14,15,16,17,18,19,20,21,22,23,24,25,0,0,0,0,
	0,0,26,27,28,29,30,31,32,33,34,35,36,
	37,38,39,40,41,42,43,44,45,46,47,48,49,50,51
};

#define REALTOBASE64LEN(real) (((real + 2 ) / 3 )* 4)

char* b64encode(char* buffer,const char* data,int len) {
	int ed = 0;
	char* p = buffer;
	
	while(len >= 3) {
		ed = (data[0] << 16) + (data[1] << 8) + data[2];
		*p++ = base64[(ed >> 18) & 0x3f];
		*p++ = base64[(ed >> 12) & 0x3f];
		*p++ = base64[(ed >> 6) & 0x3f];
		*p++ = base64[ ed & 0x3f];
		data += 3;
		len -= 3;
	}
	
	if(len == 1){
		ed = (data[0] << 8);
		*p++ = base64[(ed >> 10) & 0x3f];
		*p++ = base64[(ed >> 4) & 0x3f];
		*p++ = '=';
		*p++ = '=';
	} else if(len == 2) {
		ed = (data[0] << 16) + (data[1] << 8);
		*p++ = base64[(ed >> 18) & 0x3f];
		*p++ = base64[(ed >> 12) & 0x3f];
		*p++ = base64[(ed >> 6) & 0x3f];
		*p++ = '=';
	}
	
	*p = 0;
	
	return buffer;	
}

char* b64decode(char* buffer,const char* data,int len) {
	char* p = buffer;
	int dd = 0;

	while(len > 0) {
		dd = (dbase64[(data[0] - '+')] << 18) +
			 (dbase64[(data[1] - '+')] << 12) +
			 (dbase64[(data[2] - '+')] << 6) +
			 (dbase64[(data[3] - '+')] << 0);
			 
		*p++ = (dd >> 16);
		*p++ = (dd >> 8) & 0xff;
		*p++ = (dd) & 0xff;
		
		data += 4;
		len -= 4;
	}
	
	*p = 0;
	
	return buffer;
}