#pragma hdrstop

#include "class_AESGCM.h"

CAESGCM128::CAESGCM128()
{
    ase_128 = new CAES128();
}

CAESGCM128::~CAESGCM128()
{
    delete ase_128;
}

int CAESGCM128::gcm_encrypt(                       /* encrypt & authenticate data  */
	unsigned char data[],           /* the data buffer              */
	unsigned long data_len,         /* and its length in bytes      */
	gcm_ctx ctx[1])                 /* the mode context             */
{
	ase_128->gcm_crypt_data(data, data_len, ctx);
	ase_128->gcm_auth_data(data, data_len, ctx);
	return RETURN_GOOD;
}

int CAESGCM128::gcm_auth_header(                   /* authenticate the header      */
	const unsigned char hdr[],      /* the header buffer            */
	unsigned long hdr_len,          /* and its length in bytes      */
	gcm_ctx ctx[1])                 /* the mode context             */
{
	uint_32t cnt = 0, b_pos = (uint_32t)ctx->hdr_cnt & BLK_ADR_MASK;

	if(!hdr_len)
		return RETURN_GOOD;

	if(ctx->hdr_cnt && b_pos == 0)
		ase_128->gf_mul_hh(ctx->hdr_ghv, ctx);

	if(!((hdr - (UI8_PTR(ctx->hdr_ghv) + b_pos)) & BUF_ADRMASK))
	{
		while(cnt < hdr_len && (b_pos & BUF_ADRMASK))
			UI8_PTR(ctx->hdr_ghv)[b_pos++] ^= hdr[cnt++];

		while(cnt + BUF_INC <= hdr_len && b_pos <= BLOCK_SIZE - BUF_INC)
		{
			*UNIT_PTR(UI8_PTR(ctx->hdr_ghv) + b_pos) ^= *UNIT_PTR(hdr + cnt);
			cnt += BUF_INC; b_pos += BUF_INC;
		}

		while(cnt + BLOCK_SIZE <= hdr_len)
		{
			ase_128->gf_mul_hh(ctx->hdr_ghv, ctx);
			ase_128->xor_block_aligned(ctx->hdr_ghv, ctx->hdr_ghv, hdr + cnt);
			cnt += BLOCK_SIZE;
		}
	}
	else
	{
		while(cnt < hdr_len && b_pos < BLOCK_SIZE)
			UI8_PTR(ctx->hdr_ghv)[b_pos++] ^= hdr[cnt++];

		while(cnt + BLOCK_SIZE <= hdr_len)
		{
			ase_128->gf_mul_hh(ctx->hdr_ghv, ctx);
			ase_128->xor_block(ctx->hdr_ghv, ctx->hdr_ghv, hdr + cnt);
			cnt += BLOCK_SIZE;
		}
	}

	while(cnt < hdr_len)
	{
		if(b_pos == BLOCK_SIZE)
		{
			ase_128->gf_mul_hh(ctx->hdr_ghv, ctx);
			b_pos = 0;
		}
		UI8_PTR(ctx->hdr_ghv)[b_pos++] ^= hdr[cnt++];
	}

	ctx->hdr_cnt += cnt;
	return RETURN_GOOD;
}

int CAESGCM128::gcm_init_message(                  /* initialise a new message     */
	unsigned char iv[],       /* the initialisation vector    */
	unsigned long iv_len,           /* and its length in bytes      */
	gcm_ctx ctx[1])                 /* the mode context             */
{
	uint_32t i, n_pos = 0;
	uint_8t *p;

	memset(ctx->ctr_val, 0, BLOCK_SIZE);
	if(iv_len == CTR_POS)
	{
		memcpy(ctx->ctr_val, iv, CTR_POS); UI8_PTR(ctx->ctr_val)[15] = 0x01;
	}
	else
	{   n_pos = iv_len;
	while(n_pos >= BLOCK_SIZE)
	{
		ase_128->xor_block_aligned(ctx->ctr_val, ctx->ctr_val, iv);
		n_pos -= BLOCK_SIZE;
		iv += BLOCK_SIZE;
		ase_128->gf_mul_hh(ctx->ctr_val, ctx);
	}

	if(n_pos)
	{
		p = UI8_PTR(ctx->ctr_val);
		while(n_pos-- > 0)
			*p++ ^= *iv++;
		ase_128->gf_mul_hh(ctx->ctr_val, ctx);
	}
	n_pos = (iv_len << 3);
	for(i = BLOCK_SIZE - 1; n_pos; --i, n_pos >>= 8)
		UI8_PTR(ctx->ctr_val)[i] ^= (unsigned char)n_pos;
	ase_128->gf_mul_hh(ctx->ctr_val, ctx);
	}

	ctx->y0_val = *UI32_PTR(UI8_PTR(ctx->ctr_val) + CTR_POS);
	memset(ctx->hdr_ghv, 0, BLOCK_SIZE);
	memset(ctx->txt_ghv, 0, BLOCK_SIZE);
	ctx->hdr_cnt = 0;
	ctx->txt_ccnt = ctx->txt_acnt = 0;
	return RETURN_GOOD;
}

int CAESGCM128::gcm_init_and_key(const unsigned char key[],unsigned long key_len,gcm_ctx ctx[1])
{
	memset(ctx->ghash_h, 0, sizeof(ctx->ghash_h));

	/* set the AES key                          */
	ase_128->aes_encrypt_key(key, key_len, ctx->aes);

	/* compute E(0) (for the hash function)     */
	ase_128->aes_encrypt(UI8_PTR(ctx->ghash_h), UI8_PTR(ctx->ghash_h), ctx->aes);

	ase_128->init_4k_table(ctx->ghash_h, ctx->gf_t4k);

	return RETURN_GOOD;
}
int CAESGCM128::gcm_compute_tag(                   /* compute authentication tag   */
	unsigned char tag[],            /* the buffer for the tag       */
	unsigned long tag_len,          /* and its length in bytes      */
	gcm_ctx ctx[1])                 /* the mode context             */
{
	uint_32t i, ln;
	gf_t tbuf;

	if(ctx->txt_acnt != ctx->txt_ccnt && ctx->txt_ccnt > 0)
		return RETURN_ERROR;

	ase_128->gf_mul_hh(ctx->hdr_ghv, ctx);
	ase_128->gf_mul_hh(ctx->txt_ghv, ctx);

	if(ctx->hdr_cnt)
	{
		ln = (uint_32t)((ctx->txt_acnt + BLOCK_SIZE - 1) / BLOCK_SIZE);
		if(ln)
		{
			memcpy(tbuf, ctx->ghash_h, BLOCK_SIZE);

			for( ; ; )
			{
				if(ln & 1)
				{
					ase_128->gf_mul(ctx->hdr_ghv, tbuf);
				}
				if(!(ln >>= 1))
					break;
				ase_128->gf_mul(tbuf, tbuf);
			}
		}
	}

	i = BLOCK_SIZE; 

	{   uint_32t tm = ((uint_32t)ctx->txt_acnt) << 3;
	while(i-- > 0)
	{
		UI8_PTR(ctx->hdr_ghv)[i] ^= UI8_PTR(ctx->txt_ghv)[i] ^ (unsigned char)tm;
		tm = (i == 8 ? (((uint_32t)ctx->hdr_cnt) << 3) : tm >> 8);
	}
	}


	ase_128->gf_mul_hh(ctx->hdr_ghv, ctx);

	memcpy(ctx->enc_ctr, ctx->ctr_val, BLOCK_SIZE);
	*UI32_PTR(UI8_PTR(ctx->enc_ctr) + CTR_POS) = ctx->y0_val;
	ase_128->aes_encrypt(UI8_PTR(ctx->enc_ctr), UI8_PTR(ctx->enc_ctr), ctx->aes);
	for(i = 0; i < (unsigned int)tag_len; ++i)
		tag[i] = (unsigned char)(UI8_PTR(ctx->hdr_ghv)[i] ^ UI8_PTR(ctx->enc_ctr)[i]);

	return (ctx->txt_ccnt == ctx->txt_acnt ? RETURN_GOOD : RETURN_WARN);
}

int CAESGCM128::gcm_end(
	gcm_ctx ctx[1])
{
	memset(ctx, 0, sizeof(gcm_ctx));
	return RETURN_GOOD;
}

int CAESGCM128::gcm_decrypt(
	unsigned char data[],
	unsigned long data_len,
	gcm_ctx ctx[1])
{
	ase_128->gcm_auth_data(data, data_len, ctx);
	ase_128->gcm_crypt_data(data, data_len, ctx);
	return RETURN_GOOD;
}
void CAESGCM128::Encrypt_StringData(char type, char* pKey,char* pTitle, char* pFC,char* pAK,unsigned int ptx_len,char* pOutTag)
{
	unsigned char   key[16], iv[12], hdr[17], tag[16];
	int             key_len, iv_len, tag_len,hdr_len;
	tag_len=12;
	key_len=16;
	iv_len=12;
	hdr_len=17;
	
	memcpy(key,pKey,key_len);
	memcpy(iv,pTitle,8);
	memcpy((unsigned char*)&iv[8],pFC,4);
	memcpy(hdr,(unsigned char*)&type,1);
	memcpy((unsigned char*)&hdr[1],pAK,16);

//	if(ptx_len>=_MaxText_Length_)return -1;
		
	gcm_init_and_key(key, key_len,contx);
	gcm_init_message(iv, iv_len, contx);
	if(type==0x10)
	{
		hdr_len += ptx_len;
		gcm_auth_header(S, hdr_len, contx);
		ptx_len = 0;
	}
	else if(type == 0x20)
	{	
		hdr_len=0;
		gcm_auth_header(hdr, hdr_len, contx);
	}
	else
		gcm_auth_header(hdr, hdr_len, contx);

	gcm_encrypt(S, ptx_len, contx);
	
	gcm_compute_tag(tag, tag_len,contx);
	memcpy(pOutTag,tag, tag_len);
	gcm_end(contx);
}

void CAESGCM128::Decrypt_StringData(char type, char* pKey,char* pTitle, char* pFC,char* pAK,unsigned int ctx_len,char* pTag)
{
	unsigned char   key[16], iv[12], hdr[17],  tbuf[16];
	int             key_len, iv_len,tag_len,hdr_len;

	key_len=16;
	hdr_len=17;
	iv_len=12;
	tag_len=12;

	memcpy(key,pKey,key_len);
	memcpy(iv,pTitle,8);
	memcpy((unsigned char*)&iv[8],pFC,4);
	memcpy(hdr,(unsigned char*)&type,1);
	memcpy((unsigned char*)&hdr[1],pAK,16);

	gcm_init_and_key(key, key_len,contx);
	gcm_init_message(iv, iv_len, contx);
	gcm_auth_header(hdr, hdr_len, contx);
	gcm_decrypt(S, ctx_len,(gcm_ctx*) contx);
	gcm_compute_tag(tbuf, tag_len,(gcm_ctx*) contx);
	
	memcpy(pTag,tbuf,tag_len);
	gcm_end((gcm_ctx*)contx);
}

int CAESGCM128::aes_wrap(const unsigned char *kek, int n, const unsigned char *plain, unsigned char *cipher,aes_encrypt_ctx aes[1])
{
	unsigned char *a, *r, b[16];
	int i, j;


	a = cipher;
	r = cipher + 8;

	memset(a, 0xa6, 8);
	memcpy(r, plain, 8 * n);


	ase_128->aes_encrypt_key(kek, 16, aes);

	for (j = 0; j <= 5; j++) {
		r = cipher + 8;
		for (i = 1; i <= n; i++) {
			memcpy(b, a, 8);
			memcpy(b + 8, r, 8);
			ase_128->aes_encrypt(b, b, aes);
			memcpy(a, b, 8);
			a[7] ^= n * j + i;
			memcpy(r, b + 8, 8);
			r += 8;
		}
	}
	memset(aes, 0, sizeof(aes_encrypt_ctx));
        return 0;
}

int CAESGCM128::aes_unwrap(const unsigned char *kek, int n, const unsigned char *cipher, unsigned char *plain,aes_decrypt_ctx aes[1])
{
	unsigned char a[8], *r, b[16];
	int i, j;

	/* 1) Initialize variables. */
	memcpy(a, cipher, 8);
	r = plain;
	memcpy(r, cipher + 8, 8 * n);

	ase_128->aes_decrypt_key(kek, 16,aes);
	if (aes == NULL)
		return -1;

	/* 2) Compute intermediate values.
	 * For j = 5 to 0
	 *     For i = n to 1
	 *         B = AES-1(K, (A ^ t) | R[i]) where t = n*j+i
	 *         A = MSB(64, B)
	 *         R[i] = LSB(64, B)
	 */
	for (j = 5; j >= 0; j--) {
		r = plain + (n - 1) * 8;
		for (i = n; i >= 1; i--) {
			memcpy(b, a, 8);
			b[7] ^= n * j + i;

		    memcpy(b + 8, r, 8);
			ase_128->aes_decrypt(b, b, aes);
		//	aes_decrypt(ctx, b, b);
			memcpy(a, b, 8);
			memcpy(r, b + 8, 8);
			r -= 8;
		}
	}
	memset(aes, 0, sizeof(aes_decrypt_ctx));

	/* 3) Output results.
	 *
	 * These are already in @plain due to the location of temporary
	 * variables. Just verify that the IV matches with the expected value.
	 */
	for (i = 0; i < 8; i++) {
		if (a[i] != 0xa6)
			return -1;
	}

	return 0;
}

int CAESGCM128::aes_wrap_String(char* pKey)
{
	aes_encrypt_ctx contx1[1];
//	unsigned char kek[16];
//	unsigned char nKeyLen=16;
	int nRet;
	
//	nPlainLen=16;
//	memset(&kek,0,16);
//	memcpy(kek,pKey,nKeyLen);
	
	nRet=aes_wrap((const unsigned char*)pKey,2,S,(unsigned char *)&S[57],contx1);
	return nRet;
}

int CAESGCM128::aes_unwrap_string(char* pKey)
{
	aes_decrypt_ctx contx1[1];
	int nRet;
	nRet=aes_unwrap((const unsigned char*)pKey,2,S,(unsigned char *)&S[57],contx1);
	return nRet;
}