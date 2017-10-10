//---------------------------------------------------------------------------

#ifndef ObjsListH
#define ObjsListH
//---------------------------------------------------------------------------
#include <vector>
#include "class_Stack62056.h"

class StackList
{
    public:
        StackList();
        ~StackList();
    public:
        void Add();
        void Delete();
        unsigned int Find();
    private:
        std::vector < CStack62056 * > svec;
};
#endif
