#ifndef V8SAFECALL_H
#define V8SAFECALL_H

#include <exception>
#include <v8.h>

template<void (*FUNC)(const v8::FunctionCallbackInfo<v8::Value>&)>
void V8SafeCall(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    try
    {
        FUNC(args);
    }
    catch(const std::exception& ex)
    {
        v8::ThrowException(v8::Exception::Error(v8::String::New(ex.what())));
    }
}

#endif
