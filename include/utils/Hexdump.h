#pragma once
#include <functional>
#include <mutex>
//////////////////////////////////////////////////////////////////////////////////////////////////
// HEXDUMP
/////////////////////////////////////////////////////////////////////////////////////////////////
static const char __HEXDUMP_HEX[] = "0123456789abcdef";

static const char __HEXDUMP_ASCII[] = {
	'.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',
	'.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',  '.',
	'.',  '!',  '\"',  '#',  '$',  '%',  '&',  '\'',  '(',  ')',  '*',  '+',  ',',  '-',  '.', '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  '[',  '\\',  ']',  '^',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '{',  '|',  '}',  '~',  '?',
	'?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
	'?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',  '?',
};

class HexDump {
public:
	HexDump(int alloc = 1024) : alloc_(alloc) {
		allocBuffer_ = new char[alloc_];
	}
	~HexDump() {
		if (allocBuffer_) {
			delete[] allocBuffer_;
			allocBuffer_ = nullptr;
		}	
	}

	/*
	8 + (3 * 8 + 1) * 2 + 2 + 1 + 17 + 1 + 1
	*/
	inline void hexdump(const unsigned char* buffer, int len,std::function<void (const char*)> print)
	{
		int alloc = ((len + 15) >> 4) * (8 + (3 * 8 + 1) * 2 + 2 + 1 + 17 + 1 + 1) + 1;

		mutex_.lock();
		if (alloc > alloc_) {
			delete[] allocBuffer_;
			allocBuffer_ = new char[alloc];
			alloc_ = alloc;
		}

		char* pointerBuffer = allocBuffer_;
		int line = 0;
		while (len > 16) {
			pointerBuffer = push_line(pointerBuffer, buffer, line);
			buffer += 16;
			len -= 16;
			line += 16;
		}
		if (len != 0) {
			pointerBuffer = push_unfinish_line(pointerBuffer, buffer, buffer + len, line);
		}
		*pointerBuffer = 0;

		if (print) {
			print(allocBuffer_);
		}
		mutex_.unlock();
	}
private:
	char* allocBuffer_;
	int alloc_;
	std::mutex mutex_;

	inline char* push_uint32(char* buffer, unsigned int num)
	{
		*buffer++ = __HEXDUMP_HEX[(num >> 28) & 0xf];
		*buffer++ = __HEXDUMP_HEX[(num >> 24) & 0xf];
		*buffer++ = __HEXDUMP_HEX[(num >> 20) & 0xf];
		*buffer++ = __HEXDUMP_HEX[(num >> 16) & 0xf];
		*buffer++ = __HEXDUMP_HEX[(num >> 12) & 0xf];
		*buffer++ = __HEXDUMP_HEX[(num >> 8) & 0xf];
		*buffer++ = __HEXDUMP_HEX[(num >> 4) & 0xf];
		*buffer++ = __HEXDUMP_HEX[(num) & 0xf];
		return buffer;
	}
	inline char* push_byte(char* buffer, unsigned char num)
	{
		*buffer++ = __HEXDUMP_HEX[(num >> 4) & 0xf];
		*buffer++ = __HEXDUMP_HEX[(num) & 0xf];
		return buffer;
	}
	inline char* push_line(char* buffer, const unsigned char* src, unsigned int line)
	{
		int index = 0;
		buffer = push_uint32(buffer, line);

		*buffer++ = ' ';
		for (index = 0; index < 8; index++) {
			*buffer++ = ' ';
			buffer = push_byte(buffer, *src++);
		}

		*buffer++ = ' ';
		for (index = 0; index < 8; index++) {
			*buffer++ = ' ';
			buffer = push_byte(buffer, *src++);
		}

		*buffer++ = ' ';
		*buffer++ = ' ';

		*buffer++ = '|';
		src -= 16;

		for (index = 0; index < 8; index++) {
			*buffer++ = __HEXDUMP_ASCII[*src++];
		}
		*buffer++ = ' ';
		for (index = 0; index < 8; index++) {
			*buffer++ = __HEXDUMP_ASCII[*src++];
		}

		*buffer++ = '|';
		*buffer++ = '\n';
		return buffer;
	}
	inline char* push_unfinish_line(char* buffer, const unsigned char* begin, const unsigned char* end, unsigned int line)
	{
		int index;
		char* p = buffer + (8 + (3 * 8 + 1) * 2 + 2);
		const unsigned char* originBegin = begin;

		buffer = push_uint32(buffer, line);

		if (end - begin > 8) {
			*buffer++ = ' ';
			for (index = 0; index < 8; index++) {
				*buffer++ = ' ';
				buffer = push_byte(buffer, *begin++);
			}
		}
		*buffer++ = ' ';
		while (begin < end) {
			*buffer++ = ' ';
			buffer = push_byte(buffer, *begin++);
		}

		while (buffer < p) *buffer++ = ' ';

		*buffer++ = '|';
		if (end - originBegin > 8) {
			begin = originBegin + 8;
			while (originBegin < begin) *buffer++ = __HEXDUMP_ASCII[*originBegin++];
			*buffer++ = ' ';
		}
		while (originBegin < end) *buffer++ = __HEXDUMP_ASCII[*originBegin++];
		*buffer++ = '|';
		*buffer++ = '\n';
		*buffer++ = 0;
		return buffer;
	}
};
