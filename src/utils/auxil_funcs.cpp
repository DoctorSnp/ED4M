#include "auxil_funcs.h"

int BitState(int array, int bitNum)
{
    if (array & (1<< bitNum))
        return 1;
    return 0;
}

void setBitState(char* array, int bitNum, int whatDo)
{
    if (whatDo)
        *array |= 1UL << bitNum;
    else
        *array &= ~(1UL << bitNum);
}
