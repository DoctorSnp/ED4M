
//#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <conio.h>
#include <errno.h>


#include "src/ts.h"
#include "utils.h"


void  Printer_print(Engine *eng, int dbgLevel, const wchar_t *format, ...) noexcept
{
    va_list args;
    va_start(args, format);
    wchar_t text[2048];
    vswprintf_s(text, sizeof (text), format, args);
    va_end(args);

    eng->ShowMessage(dbgLevel, text);
}


int bitIsSet(int array, int bitNum)
{
    if (array & (1<< bitNum))
        return 1;
    return 0;
}
