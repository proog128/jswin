#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <v8.h>
#include <windows.h>

inline std::vector<char> ToAnsi(v8::Handle<v8::String> V8String)
{
    std::vector<char> Ansi;

    v8::String::Value Utf16(V8String);
    const wchar_t* pUtf16 = (const wchar_t*)*Utf16;
    int len = WideCharToMultiByte(CP_ACP, 0, pUtf16, -1, NULL, 0, "?", NULL);
    if(len > 1)
    {
        Ansi.resize(len);
        WideCharToMultiByte(CP_ACP, 0, pUtf16, -1, Ansi.data(), Ansi.size(), "?", NULL);
    }
    return Ansi;
}

inline std::vector<wchar_t> ToWideChar(v8::Handle<v8::String> V8String)
{
    std::vector<wchar_t> WideChar;

    v8::String::Utf8Value Utf8(V8String);
    const char* pUtf8 = (const char*)*Utf8;
    int len = MultiByteToWideChar(CP_UTF8, 0, pUtf8, -1, NULL, 0);
    if(len > 1)
    {
        WideChar.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, pUtf8, -1, WideChar.data(), WideChar.size());
    }
    return WideChar;
}

#endif
