#ifndef _CPPUTILS_BASE64_H
#define _CPPUTILS_BASE64_H

namespace CppUtils
{
namespace Crypto
{

extern char* b64encode(char* buffer,const char* data,int len);

extern char* b64decode(char* buffer,const char* data,int len);

}
}
#endif