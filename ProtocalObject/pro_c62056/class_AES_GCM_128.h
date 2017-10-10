//---------------------------------------------------------------------------

#ifndef class_AES_GCM_128H
#define class_AES_GCM_128H

#include "aes_gcm.h"
#include "class_AES_128.h"
//---------------------------------------------------------------------------
class CAESGCM128
{
    public:
        CAESGCM128();
        ~CAESGCM128();

    public:
        void AES_GCM_Encryption(unsigned char Type, unsigned char SC, unsigned char *Sys_T,
                                            unsigned char *FC, unsigned char *EK, unsigned char *AK, const unsigned char *TextBefore, unsigned short TextBeforeLen);
    private:
        void Muliplication(unsigned char *Z, const unsigned char *X, const unsigned char *Y);
        void GHASH(unsigned char *Y, const unsigned char *X, unsigned short Len, const unsigned char *H);
        void GCTR(unsigned char *Output, const unsigned char *ICB,
                                const unsigned char *X, unsigned char X_Len, const unsigned char *K);
        void CIPH(unsigned char *C, unsigned char *CB, const unsigned char *K);
        void inc(unsigned char *Input, unsigned short BitLen);
        unsigned char GetBit(const unsigned char *Input, unsigned char index);
        unsigned char GetLSB(unsigned char Input);
        unsigned char RotationRight(unsigned char *Input);
        void ExclusiveOR(unsigned char *Output, const unsigned char *Input1, const unsigned char *Input2, unsigned int Len);

    public:
        //-----------------------------------------加密结果-------------------------------------------
        unsigned char T[T_LEN];// = {0x00};
        unsigned char TextAfter[CIPHERTEXT_LEN];
        unsigned short TextAfterLen;// = 0;
    private:


        //-----------------------------------------中间处理数据-------------------------------------------
        unsigned char IV[IV_LEN];// = {0x00};
        unsigned char SH[SH_LEN];// = {0x00};

    private:
        CAES128 *ase_128;
};
#endif
