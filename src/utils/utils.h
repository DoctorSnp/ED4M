#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <cctype>
#include <cwctype>


#include "src/ts.h"
#include "src/shared_structs.h"


#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))


#define IS_GREEN(sig)   ( sig == SIGASP_CLEAR_1) || (sig == SIGASP_CLEAR_2 )
#define IS_YELLOW(sig)  ( sig == SIGASP_APPROACH_1) || (sig == SIGASP_APPROACH_2) || (sig == SIGASP_APPROACH_3 )
#define IS_KG(sig)      ( sig == SIGASP_STOP_AND_PROCEED )
#define IS_RED(sig)     ( sig == SIGASP_STOP) || ( sig == SIGASP_BLOCK_OBSTRUCTED )
#define IS_WHITE(sig)   ( sig == SIGASP_RESTRICTING )

void  Printer_print(Engine *eng, int dbgLevel, const wchar_t *format, ...) noexcept;

int bitIsSet(int array, int bitNum);

void toUpper(std::basic_string<wchar_t>& s);
wchar_t* upperCase(wchar_t * sourceStr, int *res);


en_SignColors m_getSignColor(int sig);
int m_getSignCode(st_ALSN *alsn);

#endif // UTILS_H
