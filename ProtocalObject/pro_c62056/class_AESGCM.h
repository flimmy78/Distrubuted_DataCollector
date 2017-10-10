#include "Public.h"
#include "class_AES_128.h"
#ifndef class_AESGCM
#define class_AESGCM
//---------------------------------------------------------------------------
class CAESGCM128
{
    public:
        CAESGCM128();
        ~CAESGCM128();

    public:
	void Decrypt_StringData(char type, char* pKey,char* pTitle, char* pFC,char* pHDR,unsigned int ctx_len,char* pTag);
	void Encrypt_StringData(char type, char* pKey,char* pTitle, char* pFC,char* pAK,unsigned int ptx_len,char* pOutTag);
	int aes_wrap_String(char* pKey);
	int aes_unwrap_string(char* pKey);
    private:
            int gcm_init_and_key(const unsigned char key[],unsigned long key_len,gcm_ctx ctx[1]);

            int gcm_end(gcm_ctx ctx[1]);

		int gcm_init_message(                  /* initialise a new message     */
            unsigned char iv[],       /* the initialisation vector    */
            unsigned long iv_len,           /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

		int gcm_auth_header(                   /* authenticate the header      */
            const unsigned char hdr[],      /* the header buffer            */
            unsigned long hdr_len,          /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

		int gcm_encrypt(                       /* encrypt & authenticate data  */
            unsigned char data[],           /* the data buffer              */
            unsigned long data_len,         /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

		int gcm_decrypt(                       /* authenticate & decrypt data  */
            unsigned char data[],           /* the data buffer              */
            unsigned long data_len,         /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

		int gcm_compute_tag(                   /* compute authentication tag   */
            unsigned char tag[],            /* the buffer for the tag       */
            unsigned long tag_len,          /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

		int aes_unwrap(const unsigned char *kek, int n, const unsigned char *cipher, unsigned char *plain,aes_decrypt_ctx aes[1]);
		int aes_wrap(const unsigned char *kek, int n, const unsigned char *plain, unsigned char *cipher,aes_encrypt_ctx aes[1]);


    public:
        
	 unsigned char T[T_LEN];// = {0x00};
	 unsigned char S[S_LEN];
	 //unsigned char TextAfter[CIPHERTEXT_LEN];
	 //unsigned short TextAfterLen;// = 0;
    private:
	gcm_ctx contx[1];
        //-----------------------------------------中间处理数据-------------------------------------------

   private:
        CAES128 *ase_128;
};
#endif

