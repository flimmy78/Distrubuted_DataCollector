//---------------------------------------------------------------------------

#ifndef class_AES_128H
#define class_AES_128H

#include "Public.h"
#define N_ROUND (11)
//---------------------------------------------------------------------------
class CAES128
{
    public:
        CAES128();
        ~CAES128();

    public:
		int gcm_auth_data(                     /* authenticate ciphertext data */
             const unsigned char data[],     /* the data buffer              */
             unsigned long data_len,         /* and its length in bytes      */
             gcm_ctx ctx[1]);                /* the mode context             */

 		int gcm_crypt_data(                    /* encrypt or decrypt data      */
             unsigned char data[],           /* the data buffer              */
             unsigned long data_len,         /* and its length in bytes      */
             gcm_ctx ctx[1]);                /* the mode context             */    

		void gf_mul_hh(gf_t a, gcm_ctx ctx[1]);
		void xor_block_aligned(void *r, const void *p, const void *q);
		void xor_block(void *r, const void* p, const void* q);
		int aes_encrypt(const unsigned char *in, unsigned char *out, const aes_encrypt_ctx cx[1]);
		int aes_encrypt_key(const unsigned char *key, int key_len, aes_encrypt_ctx cx[1]);
		void init_4k_table(const gf_t g, gf_t4k_a  t);
		void gf_mul(gf_t a, const gf_t b);
		int aes_decrypt_key(const unsigned char *key, int key_len, aes_decrypt_ctx cx[1]);
		int aes_decrypt(const unsigned char *in, unsigned char *out, const aes_decrypt_ctx cx[1]);

private:   
		void copy_block_aligned(void *p, const void *q);
		gf_decl void gf_mulx8_lb(gf_t x);
		void gf_mul_4k(gf_t a, const gf_t4k_a t, gf_t r);
		int aes_decrypt_key128(const unsigned char *key, aes_decrypt_ctx cx[1]);
			
		static inline unsigned char byte(const unsigned long x, const unsigned n);
		gf_decl void gf_mulx1_lb(gf_t r, const gf_t x);
		unsigned long ror32(unsigned long data,unsigned char rotation);
		int aes_encrypt_key128(const unsigned char *key, aes_encrypt_ctx cx[1]);
        //----------------------------------------------------------

        
};
#endif
