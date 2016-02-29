#ifndef __CPPUTILS_AES_CRYPTO_
#define __CPPUTILS_AES_CRYPTO_

#define AES_MIN_KEY_SIZE	16
#define AES_MAX_KEY_SIZE	32
#define AES_KEYSIZE_128		16
#define AES_KEYSIZE_192		24
#define AES_KEYSIZE_256		32
#define AES_BLOCK_SIZE		16
#define AES_MAX_KEYLENGTH	(15 * 16)
#define AES_MAX_KEYLENGTH_U32	(AES_MAX_KEYLENGTH / sizeof(u32))

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
	Aes();
	virtual ~Aes();
};
#endif
