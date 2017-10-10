//---------------------------------------------------------------------------

#ifndef Class_StackListH
#define Class_StackListH
//---------------------------------------------------------------------------
#include <string>
#include <vector>
#include "class_Stack62056.h"

class StackList
{
    public:
        StackList();
        ~StackList();
    public:
        CStack62056 * Add(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr);
        bool Delete(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr);
        CStack62056 * Find(SUPPORT_LAYER SupportLayerType, unsigned char ID[ID_LEN], ADDR Addr);
    private:
        std::vector <CStack62056 *> svec;
};
#endif
