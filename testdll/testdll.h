#ifndef TESTDLL_H
#define TESTDLL_H

typedef void (__cdecl *Callback)(int a, int b);
typedef void (__stdcall *Callback2)(int a, int b);

__declspec(dllexport) int __cdecl Sum(int a, int b, Callback callback);
__declspec(dllexport) int __stdcall Sum2(int a, int b, Callback2 callback);

typedef struct
{
    int a;
} IntStruct;
__declspec(dllexport) int __cdecl Struct(IntStruct* p);

#endif
