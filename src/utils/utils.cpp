
//#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <conio.h>
#include <errno.h>
#include <string.h>
#include <wctype.h>
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

void upperCase(wchar_t * sourceStr)
{
    int i = 0;
    wchar_t c;
    while (sourceStr[i])
     {
       c = sourceStr[i];
       sourceStr[i] = putwchar (towupper(c));
       i++;
     }
}

en_SignColors m_getSignColor(int sig)
{
    if ( (sig == SIGASP_CLEAR_1 ) || (sig == SIGASP_CLEAR_2) )
        return en_SignColors::COLOR_GREEN;
    else if ( (sig == SIGASP_APPROACH_1) || (sig == SIGASP_APPROACH_2) || (sig == SIGASP_APPROACH_3) )
        return en_SignColors::COLOR_YELLOW;
    else if (sig == SIGASP_STOP_AND_PROCEED)
        return en_SignColors::COLOR_RD_YEL;
    else if (sig == SIGASP_RESTRICTING)
        return en_SignColors::COLOR_WHITE;
    else if ((sig == SIGASP_STOP) ||  (SIGASP_BLOCK_OBSTRUCTED))
        return en_SignColors::COLOR_RED;
    else
        return en_SignColors::COLOR_WHITE;
}

int m_getSignCode( st_ALSN *alsn)
{
    for (int i = 0; i < alsn->NumSigForw &&  i< SIGNALS_CNT; i++ )
    {
       if ( CHECK_BIT(alsn->ForwardSignalsList[i].Flags, 3) )
           continue;
       if ( CHECK_BIT(alsn->ForwardSignalsList[i].Flags, 1) )
           continue;

       //alsn->SpeedLimit.Distance = (int)alsn->ForwardSignalsList[i].Distance;
       if (IS_KG(alsn->ForwardSignalsList[i].Aspect[0]))
            return SIGASP_STOP_AND_PROCEED;
       else if (IS_YELLOW(alsn->ForwardSignalsList[i].Aspect[0]))
           return SIGASP_APPROACH_2;
       else if (IS_GREEN(alsn->ForwardSignalsList[i].Aspect[0]))
           return SIGASP_CLEAR_2;
       else
           continue;
    }

    if (IS_RED(alsn->ForwardSignalsList[alsn->NumSigForw-1].Aspect[0]))
        return SIGASP_STOP ;
    return  SIGASP_RESTRICTING;
}
