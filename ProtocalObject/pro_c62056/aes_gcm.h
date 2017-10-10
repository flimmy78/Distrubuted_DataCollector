#ifndef AES_GCM_H
#define AES_GCM_H
#define _MaxText_Length_ 384  //最大加解密数据长度 
#define xor_4k(i,ap,t,r) gf_mulx8(mode)(r); xor_block_aligned(r, r, t[ap[GF_INDEX(i)]])

#  define ls_box1(x,c)       four_tables(x,t_use(f,l),vf1,rf2,c)

#define v(n,i)  ((n) - (i) + 2 * ((i) & 3))

#define ls_box(x)		\
	crypto_fl_tab[0][byte(x, 0)] ^	\
	crypto_fl_tab[1][byte(x, 1)] ^	\
	crypto_fl_tab[2][byte(x, 2)] ^	\
	crypto_fl_tab[3][byte(x, 3)]

#define loop4(i)	do {		\
	ss[3] = ror32(ss[3], 8);		\
	ss[3] = ls_box(ss[3]) ^ rco_tab[i];	\
	ss[3] ^= cx->ks[4 * i];		\
	cx->ks[4 * i + 4] = ss[3];		\
	ss[3] ^= cx->ks[4 * i + 1];		\
	cx->ks[4 * i + 5] = ss[3];		\
	ss[3] ^= cx->ks[4 * i + 2];		\
	cx->ks[4 * i + 6] = ss[3];		\
	ss[3] ^= cx->ks[4 * i + 3];		\
	cx->ks[4 * i + 7] = ss[3];		\
} while (0)	

#define kdf4(k,i) \
{   ss[0] = ss[0] ^ ss[2] ^ ss[1] ^ ss[3]; \
	ss[1] = ss[1] ^ ss[3]; \
	ss[2] = ss[2] ^ ss[3]; \
	ss[4] = ls_box(ror32(ss[(i+3) % 4], 8)) ^ rco_tab[i]; \
	ss[i % 4] ^= ss[4]; \
	ss[4] ^= k[v(40,(4*(i)))];   k[v(40,(4*(i))+4)] = ff(ss[4]); \
	ss[4] ^= k[v(40,(4*(i))+1)]; k[v(40,(4*(i))+5)] = ff(ss[4]); \
	ss[4] ^= k[v(40,(4*(i))+2)]; k[v(40,(4*(i))+6)] = ff(ss[4]); \
	ss[4] ^= k[v(40,(4*(i))+3)]; k[v(40,(4*(i))+7)] = ff(ss[4]); \
}

#define kd4(k,i) \
{   ss[4] = ls_box(ror32(ss[(i+3) % 4], 8)) ^ rco_tab[i]; \
	ss[i % 4] ^= ss[4]; ss[4] = ff(ss[4]); \
	k[v(40,(4*(i))+4)] = ss[4] ^= k[v(40,(4*(i)))]; \
	k[v(40,(4*(i))+5)] = ss[4] ^= k[v(40,(4*(i))+1)]; \
	k[v(40,(4*(i))+6)] = ss[4] ^= k[v(40,(4*(i))+2)]; \
	k[v(40,(4*(i))+7)] = ss[4] ^= k[v(40,(4*(i))+3)]; \
}

#define kdl4(k,i) \
{   ss[4] = ls_box(ror32(ss[(i+3) % 4], 8)) ^ rco_tab[i]; ss[i % 4] ^= ss[4]; \
	k[v(40,(4*(i))+4)] = (ss[0] ^= ss[1]) ^ ss[2] ^ ss[3]; \
	k[v(40,(4*(i))+5)] = ss[1] ^ ss[3]; \
	k[v(40,(4*(i))+6)] = ss[0]; \
	k[v(40,(4*(i))+7)] = ss[1]; \
}


#  define s(x,c) x[c]
#define si(y,x,k,c) (s(y,c) = word_in(x, c) ^ (k)[c])
#define so(y,x,c)   word_out(y, c, s(x,c))
#define locals(y,x)     x[4],y[4]
#define l_copy(y, x)    s(y,0) = s(x,0); s(y,1) = s(x,1); \
	s(y,2) = s(x,2); s(y,3) = s(x,3);
#define state_in(y,x,k) si(y,x,k,0); si(y,x,k,1); si(y,x,k,2); si(y,x,k,3)
#define state_out(y,x)  so(y,x,0); so(y,x,1); so(y,x,2); so(y,x,3)
#define round(rm,y,x,k) rm(y,x,k,0); rm(y,x,k,1); rm(y,x,k,2); rm(y,x,k,3)
#define fwd_var(x,r,c)\
	( r == 0 ? ( c == 0 ? s(x,0) : c == 1 ? s(x,1) : c == 2 ? s(x,2) : s(x,3))\
	: r == 1 ? ( c == 0 ? s(x,1) : c == 1 ? s(x,2) : c == 2 ? s(x,3) : s(x,0))\
	: r == 2 ? ( c == 0 ? s(x,2) : c == 1 ? s(x,3) : c == 2 ? s(x,0) : s(x,1))\
	:          ( c == 0 ? s(x,3) : c == 1 ? s(x,0) : c == 2 ? s(x,1) : s(x,2)))
//#define fwd_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ four_tables(x,t_use(f,n),fwd_var,rf1,c))
//#define fwd_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ four_tables(x,t_use(f,l),fwd_var,rf1,c))

#define f_rn(bo, bi, n,k)	do {				\
	bo[n] = crypto_fn_tab[0][byte(bi[n], 0)] ^			\
		crypto_fn_tab[1][byte(bi[(n + 1) & 3], 1)] ^		\
		crypto_fn_tab[2][byte(bi[(n + 2) & 3], 2)] ^		\
		crypto_fn_tab[3][byte(bi[(n + 3) & 3], 3)] ^ *(k + n);	\
} while (0)

#define f_nround(bo, bi, k)	do {\
	f_rn(bo, bi, 0, k);	\
	f_rn(bo, bi, 1, k);	\
	f_rn(bo, bi, 2, k);	\
	f_rn(bo, bi, 3, k);	\
	k += 4;			\
} while (0)

#define f_rl(bo, bi, n,k)	do {				\
	bo[n] = crypto_fl_tab[0][byte(bi[n], 0)] ^			\
		crypto_fl_tab[1][byte(bi[(n + 1) & 3], 1)] ^		\
		crypto_fl_tab[2][byte(bi[(n + 2) & 3], 2)] ^		\
		crypto_fl_tab[3][byte(bi[(n + 3) & 3], 3)] ^ *(k + n);	\
} while (0)

#define f_lround(bo, bi, k)	do {\
	f_rl(bo, bi, 0, k);	\
	f_rl(bo, bi, 1, k);	\
	f_rl(bo, bi, 2, k);	\
	f_rl(bo, bi, 3, k);	\
} while (0)

#define i_rn(bo, bi, n,k)	do {				\
	bo[n] = crypto_in_tab[0][byte(bi[n], 0)] ^			\
		crypto_in_tab[1][byte(bi[(n + 3) & 3], 1)] ^		\
		crypto_in_tab[2][byte(bi[(n + 2) & 3], 2)] ^		\
		crypto_in_tab[3][byte(bi[(n + 1) & 3], 3)] ^ *(k + n);	\
} while (0)

#define i_nround(bo, bi, k)	do {\
	i_rn(bo, bi, 0, k);	\
	i_rn(bo, bi, 1, k);	\
	i_rn(bo, bi, 2, k);	\
	i_rn(bo, bi, 3, k);	\
	k+= 4;			\
} while (0)

#define i_rl(bo, bi, n, k)	do {			\
	bo[n] = crypto_il_tab[0][byte(bi[n], 0)] ^		\
	crypto_il_tab[1][byte(bi[(n + 3) & 3], 1)] ^		\
	crypto_il_tab[2][byte(bi[(n + 2) & 3], 2)] ^		\
	crypto_il_tab[3][byte(bi[(n + 1) & 3], 3)] ^ *(k + n);	\
} while (0)

#define i_lround(bo, bi, k)	do {\
	i_rl(bo, bi, 0, k);	\
	i_rl(bo, bi, 1, k);	\
	i_rl(bo, bi, 2, k);	\
	i_rl(bo, bi, 3, k);	\
} while (0)

#define inc_ctr(x)  \
{   int i = BLOCK_SIZE; while(i-- > CTR_POS && !++(UI8_PTR(x)[i])) ; }
#define xor_4k(i,ap,t,r) gf_mulx8(mode)(r); xor_block_aligned(r, r, t[ap[GF_INDEX(i)]])
#define key_ofs     0
#define rnd_key(n)  (kp + n * N_COLS)
#define inv_var(x,r,c)\
	( r == 0 ? ( c == 0 ? s(x,0) : c == 1 ? s(x,1) : c == 2 ? s(x,2) : s(x,3))\
	: r == 1 ? ( c == 0 ? s(x,3) : c == 1 ? s(x,0) : c == 2 ? s(x,1) : s(x,2))\
	: r == 2 ? ( c == 0 ? s(x,2) : c == 1 ? s(x,3) : c == 2 ? s(x,0) : s(x,1))\
	:          ( c == 0 ? s(x,1) : c == 1 ? s(x,2) : c == 2 ? s(x,3) : s(x,0)))
#define inv_rnd(y,x,k,c)    (s(y,c) = (k)[c] ^ four_tables(x,t_use(i,n),inv_var,rf1,c))
#define inv_lrnd(y,x,k,c)   (s(y,c) = (k)[c] ^ four_tables(x,t_use(i,l),inv_var,rf1,c))

#define TYPE_ENCRYPTION 0x00
#define TYPE_DECRYPTION 0x01
//================================================================输入================================================================
//以下均为字节数
#define MAN_ID_LEN          3                                       //厂商ID，三个大写字母，例如 ‘WSE’
#define MAN_NUM_LEN         5                                       //SN，五个十进制数字转换为十六进制
#define SYS_T_LEN           (MAN_ID_LEN + MAN_NUM_LEN)              //Man_Id || Man_Num
#define FC_LEN              4                                       //Frame Counter
#define IV_LEN              (SYS_T_LEN + FC_LEN)                    // Sys_T || FC
#define KEY_LEN             16                                      //密钥长度，字节数
#define PLAINTEXT_LEN       1280//239                //APDU的最大长度，字节数，加密后的密文长度与明文长度相等

//----------------------------------------------------------------------------------
#define SC_LEN  1                                                   //为字节数
//     BIT7              BIT6                     BIT5   BIT4             BIT3~0
//   Reserve   KeySet(unicast=0, broadcast = 1)    E      A    Security_Suite_Id(AES_GCM_128 = 0)
#define SC_U_A              0x10
#define SC_U_E              0x20
#define SC_U_AE             0x30
#define SC_B_A              0x50
#define SC_B_E              0x60
#define SC_B_AE             0x70

//================================================================中间数据================================================================
//以下均为字节数
#define SH_LEN              (SC_LEN + FC_LEN)                       //Security Header = SC || FC
#define A_A_LEN             (SC_LEN + KEY_LEN + PLAINTEXT_LEN)
#define A_AE_LEN            (SC_LEN + KEY_LEN)
#define S_LEN               (SC_LEN + KEY_LEN + PLAINTEXT_LEN + 128)
//================================================================输出================================================================
//以下均为字节数
#define T_LEN                               12
#define CIPHERTEXT_LEN                      PLAINTEXT_LEN
#define TAG_LEN                             1
#define AUTHENTICATION_APDU_LEN             (TAG_LEN + SH_LEN + CIPHERTEXT_LEN + T_LEN + 3)  //3是CIPHERTEXT_LEN的长度编码最长字节数
#define ENCRYPTION_APDU_LEN                 (TAG_LEN + SH_LEN + CIPHERTEXT_LEN + 3)
#define AUTHENTICATED_ENCRYPTION_APDU_LEN   (TAG_LEN + SH_LEN + CIPHERTEXT_LEN + T_LEN + 3)
extern unsigned char S[S_LEN];
extern unsigned char T[T_LEN] ;
extern unsigned char TextBefore[CIPHERTEXT_LEN];
#endif
