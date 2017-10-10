

#if 1 && !defined( USE_INLINING )
#  define USE_INLINING
#endif

#if defined( USE_INLINING )
#  if defined( _MSC_VER )
#    define mh_decl __inline
#  elif defined( __GNUC__ ) || defined( __GNU_LIBRARY__ )
#    define mh_decl static inline
#  else
#    define mh_decl static
#  endif
#endif

#if defined( USE_INLINING )
#  define gf_decl inline
#endif

typedef unsigned char uint_8t;
typedef unsigned short uint_16t;
typedef unsigned long uint_32t;
//typedef unsigned long long uint_64t;

#if defined( UNIT_BITS )
# undef UNIT_BITS
#endif


#if !defined( UNIT_BITS )
#    define UNIT_BITS 32
#endif

#    define VOID_RETURN  void
#    define INT_RETURN   int

#define AES_RETURN INT_RETURN

#define UI_TYPE(size)               uint_##size##t
#define UNIT_TYPEDEF(x,size)        typedef UI_TYPE(size) x
//#define BUFR_TYPEDEF(x,size,bsize)  typedef UI_TYPE(size) x[bsize / (size >> 3)]
#define UNIT_CAST(x,size)           ((UI_TYPE(size) )(x))  
#define UPTR_CAST(x,size)           ((UI_TYPE(size)*)(x))

#define AES_128     /* if a fast 128 bit key scheduler is needed    */
#define AES_VAR     /* if variable key size scheduler is needed     */
#define AES_MODES   /* if support is needed for modes               */
#define KS_LENGTH       60

#define gf_m(n,x)    gf_mulx ## n ## x
#define gf_mulx1(x)  gf_m(1,x)
#define gf_mulx4(x)  gf_m(4,x)
#define gf_mulx8(x)  gf_m(8,x)

#define GF_BYTE_LEN 16
#define GF_UNIT_LEN (GF_BYTE_LEN / (UNIT_BITS >> 3))

#define BUF_INC          (UNIT_BITS >> 3)
#define BUF_ADRMASK     ((UNIT_BITS >> 3) - 1)


#define AES_BLOCK_SIZE  16  /* the AES block size in bytes          */
#define N_COLS           4  /* the number of columns in the state   */
#define GCM_BLOCK_SIZE  AES_BLOCK_SIZE

#define RC_LENGTH   (5 * (AES_BLOCK_SIZE / 4 - 2))

#define BLOCK_SIZE      GCM_BLOCK_SIZE      /* block length                 */
#define BLK_ADR_MASK    (BLOCK_SIZE - 1)    /* mask for 'in block' address  */
#define CTR_POS         12

#  define GF_MODE_LB    /* the representation used by GCM */
#  define mode   _lb
#  define GF_INDEX(i)  (i)

#define DO_TABLES
#define FIXED_TABLES
#  define FT4_SET

#  define to_byte(x)  ((x) & 0xff)

#  define bval(x,n)     to_byte((x) >> (8 * (n)))
#  define bytes2word(b0, b1, b2, b3)  \
	(((uint_32t)(b3) << 24) | ((uint_32t)(b2) << 16) | ((uint_32t)(b1) << 8) | (b0))

#define vf1(x,r,c)  (x)
#define rf1(r,c)    (r)
#define rf2(r,c)    ((8+r-c)&3)



#if defined(FIXED_TABLES)
#define rc_data(w) {\
	w(0x01), w(0x02), w(0x04), w(0x08), w(0x10),w(0x20), w(0x40), w(0x80),\
	w(0x1b), w(0x36) }

#endif

#define WPOLY   0x011b
#define BPOLY     0x1b

#define ff(x)       four_tables(x,crypto_mn_tab,vf1,rf1,0)


typedef union
{   uint_32t l;
    uint_8t b[4];
} aes_inf;
typedef struct
{  
   uint_32t ks[KS_LENGTH];
   aes_inf inf;
} aes_encrypt_ctx;

typedef struct
{   
	uint_32t ks[KS_LENGTH];
    aes_inf inf;
} aes_decrypt_ctx;

#define MASK(x) ((x) * (UNIT_CAST(-1,UNIT_BITS) / 0xff))
#define f1_lb(n,r,x)   r[n] = (x[n] >> 1) & ~MASK(0x80) | ((x[n] << 15) \
 	| (n ? x[n-1] >> 17 : 0)) & MASK(0x80)

#define f8_lb(n,r,x)   r[n] = (x[n] << 8) | (n ? x[n-1] >> 24 : 0)

#define rep2_u2(f,r,x)    f( 0,r,x); f( 1,r,x) 
#define rep2_u4(f,r,x)    f( 0,r,x); f( 1,r,x); f( 2,r,x); f( 3,r,x) 

#define rep2_d2(f,r,x)    f( 1,r,x); f( 0,r,x) 
#define rep2_d4(f,r,x)    f( 3,r,x); f( 2,r,x); f( 1,r,x); f( 0,r,x) 

#define rep3_u2(f,r,x,y,c)  f( 0,r,x,y,c); f( 1,r,x,y,c) 
#define rep3_u4(f,r,x,y,c)  f( 0,r,x,y,c); f( 1,r,x,y,c); f( 2,r,x,y,c); f( 3,r,x,y,c) 
#define rep3_u16(f,r,x,y,c) f( 0,r,x,y,c); f( 1,r,x,y,c); f( 2,r,x,y,c); f( 3,r,x,y,c); \
	f( 4,r,x,y,c); f( 5,r,x,y,c); f( 6,r,x,y,c); f( 7,r,x,y,c); \
	f( 8,r,x,y,c); f( 9,r,x,y,c); f(10,r,x,y,c); f(11,r,x,y,c); \
	f(12,r,x,y,c); f(13,r,x,y,c); f(14,r,x,y,c); f(15,r,x,y,c)
#define four_tables(x,tab,vf,rf,c) \
	(  tab[0][bval(vf(x,0,c),rf(0,c))] \
	^ tab[1][bval(vf(x,1,c),rf(1,c))] \
	^ tab[2][bval(vf(x,2,c),rf(2,c))] \
	^ tab[3][bval(vf(x,3,c),rf(3,c))])

#define UI8_PTR(x)     UPTR_CAST(x,  8)
#define UI32_PTR(x)     UPTR_CAST(x, 32)
#define UNIT_PTR(x)     UPTR_CAST(x, UNIT_BITS)

#define  UI8_VAL(x)     UNIT_CAST(x,  8)
#define UI32_VAL(x)     UNIT_CAST(x, 32)
#define UNIT_VAL(x)     UNIT_CAST(x, UNIT_BITS)

#define f_copy(n,p,q)     p[n] = q[n]
#define f_xor(n,r,p,q,c)  r[n] = c(p[n] ^ q[n])

#  define word_in(x,c)    (*((uint_32t*)(x)+(c)))
#  define word_out(x,c,v) (*((uint_32t*)(x)+(c)) = (v))

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


