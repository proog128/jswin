#include "GlobalFunctions.h"

#include <iostream>

#include "CallbackFunction.h"
#include "Library.h"
#include "Function.h"
#include "V8SafeCall.h"
#include "V8ArrayBufferUtils.h"

void V8Print(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    for(int i=0; i<args.Length(); ++i)
    {
        v8::String::Utf8Value str(args[i]);
        std::cout << *str << " ";
    }
    std::cout << "\n";
}

void V8LoadLibrary(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(args.Length() < 1)
    {
        throw std::runtime_error("Invalid argument count");
    }

    v8::String::Utf8Value name(args[0]);
    
    Library* library = new Library(*name);
    
    v8::Persistent<v8::Object> libraryObj;
    Library::V8Wrap(library, libraryObj);
    args.GetReturnValue().Set(libraryObj);
}

void V8GetBaseAddress(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(!args[0]->IsArrayBuffer())
    {
        throw std::runtime_error("Argument is not an array buffer");
    }
    v8::Handle<v8::ArrayBuffer> arrayBuffer = v8::Handle<v8::ArrayBuffer>::Cast(args[0]);

    void* ptr = ExternalizeAutoDelete(arrayBuffer);
    args.GetReturnValue().Set(v8::Uint32::New((unsigned int)ptr));
}

void V8Exit(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    int exitCode = 0;
    if(args.Length() > 0)
    {
        exitCode = args[0]->Int32Value();
    }
    exit(exitCode);
}

void V8FromMemory(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(args.Length() < 2)
    {
        throw std::runtime_error("Invalid argument count");
    }

    void* ptr = (void*)args[0]->Uint32Value();
    int len = args[1]->Uint32Value();

    v8::Local<v8::ArrayBuffer> arrayBuffer = v8::ArrayBuffer::New(ptr, len);
    arrayBuffer->SetHiddenValue(v8::String::New("__jswin_external"), v8::External::New(ptr));

    args.GetReturnValue().Set(arrayBuffer);
}

void V8ReadString(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(args.Length() <  2)
    {
        throw std::runtime_error("Invalid argument count");
    }

    v8::String::Utf8Value argType(args[0]);
    if(!strcmp(*argType, "c"))
    {
        const uint8_t* ptr = (const uint8_t*)args[1]->Uint32Value();
        args.GetReturnValue().Set(v8::String::NewFromOneByte(v8::Isolate::GetCurrent(), ptr));
    }
    else if(!strcmp(*argType, "w"))
    {
        const uint16_t* ptr = (const uint16_t*)args[1]->Uint32Value();
        args.GetReturnValue().Set(v8::String::NewFromTwoByte(v8::Isolate::GetCurrent(), ptr));
    }
    else
    {
        throw std::runtime_error("Unknown string type");
    }
}

v8::Handle<v8::ObjectTemplate> buildGlobalObject()
{
    v8::Handle<v8::ObjectTemplate> globalObjectTemplate = v8::ObjectTemplate::New();

    globalObjectTemplate->Set("print", v8::FunctionTemplate::New(V8SafeCall<V8Print>));
    globalObjectTemplate->Set("loadLibrary", v8::FunctionTemplate::New(V8SafeCall<V8LoadLibrary>));
    globalObjectTemplate->Set("getBaseAddress", v8::FunctionTemplate::New(V8SafeCall<V8GetBaseAddress>));
    globalObjectTemplate->Set("exit", v8::FunctionTemplate::New(V8SafeCall<V8Exit>));

    globalObjectTemplate->Set("DLLProc", v8::FunctionTemplate::New(V8SafeCall<Function::V8ConstructorFunction>));
    globalObjectTemplate->Set("CallbackFunction", v8::FunctionTemplate::New(V8SafeCall<CallbackFunction::V8ConstructorFunction>));

    globalObjectTemplate->Set("fromMemory", v8::FunctionTemplate::New(V8SafeCall<V8FromMemory>));
    globalObjectTemplate->Set("readString", v8::FunctionTemplate::New(V8SafeCall<V8ReadString>));

    return globalObjectTemplate;
}

