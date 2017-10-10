#include "AESHead.h"

#if defined(__cplusplus)
extern "C"
{
#endif


#ifndef RET_TYPE_DEFINED
typedef int  ret_type;
#endif

#ifndef RETURN_GOOD
# define RETURN_WARN      1
# define RETURN_GOOD      0
# define RETURN_ERROR    -1
#endif


UNIT_TYPEDEF(gcm_unit_t, UNIT_BITS);
//BUFR_TYPEDEF(gcm_buf_t, UNIT_BITS, AES_BLOCK_SIZE);

UNIT_TYPEDEF(gf_unit_t, UNIT_BITS);
//BUFR_TYPEDEF(gf_t, UNIT_BITS, GF_BYTE_LEN);
typedef uint_32t gf_t[4];
//typedef uint_32t gcm_buf_t[4];
typedef gf_t    gf_t4k_a[256];
typedef gf_t    (*gf_t4k_t);

/* The GCM-AES  context  */
#define TABLES_4K



typedef struct
{
	gf_t4k_a	    gf_t4k; 
	uint_32t       ctr_val[4];
	uint_32t       enc_ctr[4];
	uint_32t       hdr_ghv[4];
	uint_32t       txt_ghv[4];
	uint_32t            ghash_h[4];
	aes_encrypt_ctx  aes[1];
	uint_32t        y0_val;
	uint_32t        hdr_cnt;
	uint_32t        txt_ccnt;
	uint_32t        txt_acnt;
}gcm_ctx;


/* The following calls handle mode initialisation, keying and completion    */

ret_type gcm_init_and_key(                  /* initialise mode and set key  */
            const unsigned char key[],      /* the key value                */
            unsigned long key_len,          /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

ret_type gcm_end(                           /* clean up and end operation   */
            gcm_ctx ctx[1]);                /* the mode context             */

/* The following calls handle complete messages in memory as one operation  */

ret_type gcm_encrypt_message(               /* encrypt an entire message    */
            const unsigned char iv[],       /* the initialisation vector    */
            unsigned long iv_len,           /* and its length in bytes      */
            const unsigned char hdr[],      /* the header buffer            */
            unsigned long hdr_len,          /* and its length in bytes      */
            unsigned char msg[],            /* the message buffer           */
            unsigned long msg_len,          /* and its length in bytes      */
            unsigned char tag[],            /* the buffer for the tag       */
            unsigned long tag_len,          /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

                                /* RETURN_GOOD is returned if the input tag */
                                /* matches that for the decrypted message   */
ret_type gcm_decrypt_message(               /* decrypt an entire message    */
            const unsigned char iv[],       /* the initialisation vector    */
            unsigned long iv_len,           /* and its length in bytes      */
            const unsigned char hdr[],      /* the header buffer            */
            unsigned long hdr_len,          /* and its length in bytes      */
            unsigned char msg[],            /* the message buffer           */
            unsigned long msg_len,          /* and its length in bytes      */
            const unsigned char tag[],      /* the buffer for the tag       */
            unsigned long tag_len,          /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

/* The following calls handle messages in a sequence of operations followed */
/* by tag computation after the sequence has been completed. In these calls */
/* the user is responsible for verfiying the computed tag on decryption     */

ret_type gcm_init_message(                  /* initialise a new message     */
            const unsigned char iv[],       /* the initialisation vector    */
            unsigned long iv_len,           /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

ret_type gcm_auth_header(                   /* authenticate the header      */
            const unsigned char hdr[],      /* the header buffer            */
            unsigned long hdr_len,          /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

ret_type gcm_encrypt(                       /* encrypt & authenticate data  */
            unsigned char data[],           /* the data buffer              */
            unsigned long data_len,         /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

ret_type gcm_decrypt(                       /* authenticate & decrypt data  */
            unsigned char data[],           /* the data buffer              */
            unsigned long data_len,         /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

ret_type gcm_compute_tag(                   /* compute authentication tag   */
            unsigned char tag[],            /* the buffer for the tag       */
            unsigned long tag_len,          /* and its length in bytes      */
            gcm_ctx ctx[1]);                /* the mode context             */

/*  The use of the following calls should be avoided if possible because 
    their use requires a very good understanding of the way this encryption 
    mode works and the way in which this code implements it in order to use 
    them correctly.

    The gcm_auth_data routine is used to authenticate encrypted message data.
    In message encryption gcm_crypt_data must be called before gcm_auth_data
    is called since it is encrypted data that is authenticated.  In message
    decryption authentication must occur before decryption and data can be
    authenticated without being decrypted if necessary.

    If these calls are used it is up to the user to ensure that these routines
    are called in the correct order and that the correct data is passed to 
    them.

    When gcm_compute_tag is called it is assumed that an error in use has
    occurred if both encryption (or decryption) and authentication have taken
    place but the total lengths of the message data respectively authenticated
    and encrypted are not the same. If authentication has taken place but 
    there has been no corresponding encryption or decryption operations (none
    at all) only a warning is issued. This should be treated as an error if it 
    occurs during encryption but it is only signalled as a warning as it might 
    be intentional when decryption operations are involved (this avoids having
    different compute tag functions for encryption and decryption). Decryption
    operations can be undertaken freely after authetication but if the tag is
    computed after such operations an error will be signalled if the lengths
    of the data authenticated and decrypted don't match.
*/

 ret_type gcm_auth_data(                     /* authenticate ciphertext data */
             const unsigned char data[],     /* the data buffer              */
             unsigned long data_len,         /* and its length in bytes      */
             gcm_ctx ctx[1]);                /* the mode context             */

 ret_type gcm_crypt_data(                    /* encrypt or decrypt data      */
             unsigned char data[],           /* the data buffer              */
             unsigned long data_len,         /* and its length in bytes      */
             gcm_ctx ctx[1]);                /* the mode context             */

//////////////////////////////////////////////////////////////////////////

//�ӽ��ܶ���ӿں���
int Decrypt_StringData(char type, char* pKey,char* pTitle, char* pFC,char* pHDR,unsigned int ctx_len,char* pTag);
int Encrypt_StringData(char type, char* pKey,char* pTitle, char* pFC,char* pAK,unsigned int ptx_len,char* pOutTag);

void GetBCDFrom16Xchar(char *fromText,unsigned char *toData,int toDatalen);
int CopyCharToByte(char* pfrom,unsigned char* todata,int datalen);

//��Կ���ܺ���
//int aes_wrap_String(char* pKey);
//��Կ���ܺ���
int aes_unwrap_string(char* pKey);
//////////////////////////////////////////////////////////////////////////


#if defined(__cplusplus)
}
#endif
