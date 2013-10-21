#ifndef LIBRARY_H
#define LIBRARY_H

#include <Windows.h>
#include <string>
#include <v8.h>

#include "Function.h"

class Library
{
public:
    Library(const std::string& libraryName);
    ~Library();

    Function* getProc(const std::string& procName, const std::string& signature, Function::CallingConvention callingConvention);

    static void V8Init();
    static v8::Handle<v8::FunctionTemplate> V8Constructor;

    static void V8Wrap(Library* library, v8::Persistent<v8::Object>& wrappedObj);
    static void V8WeakCallback(v8::Isolate* isolate, v8::Persistent<v8::Object>* object, Library* parameter);

    static void V8GetProc(const v8::FunctionCallbackInfo<v8::Value>& args);

private:
    std::string libraryName;
    HMODULE moduleHandle;
};

#endif
