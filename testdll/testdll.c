#include "testdll.h"
#include <stdio.h>

__declspec(dllexport) int __cdecl Sum(int a, int b, Callback callback)
{
    int r = callback(a, b);
    printf("[%d]", r);
	return a + b;
}

__declspec(dllexport) int __stdcall Sum2(int a, int b, Callback2 callback)
{
    int r = callback(a, b);
    printf("[%d]", r);
	return a + b;
}

__declspec(dllexport) int __cdecl Struct(IntStruct* p)
{
    return p ? p->a : -1;
}
