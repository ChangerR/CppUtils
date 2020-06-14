#ifndef __CPPUTILS_AES_CRYPTO_
#define __CPPUTILS_AES_CRYPTO_

#define AES_MIN_KEY_SIZE	16
#define AES_MAX_KEY_SIZE	32
#define AES_KEYSIZE_128		16
#define AES_KEYSIZE_192		24
#define AES_KEYSIZE_256		32
#define AES_BLOCK_SIZE		16
#define AES_MAX_KEYLENGTH	(15 * 16)
#define AES_MAX_KEYLENGTH_U32	(AES_MAX_KEYLENGTH / sizeof(unsigned int))

namespace CppUtils
{
namespace Crypto
{
	
/**
 * ror32 - rotate a 32-bit value right
 * @word: value to rotate
 * @shift: bits to roll
 */
static inline unsigned int ror32(unsigned int word, unsigned int shift)
{
	return (word >> shift) | (word << (32 - shift));
}

struct crypto_aes_ctx {
	unsigned int key_enc[AES_MAX_KEYLENGTH_U32];
	unsigned int key_dec[AES_MAX_KEYLENGTH_U32];
	unsigned int key_length;
};

class Aes {
public:
	/**
	 * Aes Construct function
	 *
	 * No return
	 */
	Aes();
	virtual ~Aes();

	/**
	 * setKey - Set Aes key
	 * @key  Your encrypt key
	 * @len  Your encrypt key len, You can choose 16(128) 24(192) 32(256)
	 *
	 * Returns true on success, false on failure.
	 */

	bool setKey(const unsigned char* key,int len);

	/**
	 * decrypt - decrypt 
	 *
	 */

	bool decrypt(const unsigned char* data,int len);

	/**
	 * decrypt
	 */
	bool encrypt(const unsigned char* data,int len);

	/**
	 * copyResult
	 */
	int copyResult(unsigned char* data,int *len);

	/**
	 * getResult
	 */
	unsigned char* getResult(int* len);

private:
	struct crypto_aes_ctx _ctx;

	unsigned char* _buffer;

	int _alloc,_pos;

	bool _isSetKey;
};

}
}
#endif
