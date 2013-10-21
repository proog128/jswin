#include "Library.h"

#include <stdexcept>

#include "V8SafeCall.h"

v8::Handle<v8::FunctionTemplate> Library::V8Constructor;

Library::Library(const std::string& libraryName)
    : libraryName(libraryName)
{
    moduleHandle = LoadLibraryA(libraryName.c_str());
}

Library::~Library()
{
    FreeLibrary(moduleHandle);
}

Function* Library::getProc(const std::string& procName, const std::string& signature, Function::CallingConvention callingConvention)
{
    void* address = GetProcAddress(moduleHandle, procName.c_str());
    if(address == NULL)
    {
        throw std::runtime_error("Function not found");
    }

    return new Function(signature, callingConvention, address);
}

void Library::V8Init()
{
    V8Constructor = v8::FunctionTemplate::New();
    V8Constructor->InstanceTemplate()->SetInternalFieldCount(1);
    V8Constructor->PrototypeTemplate()->Set("getProc", v8::FunctionTemplate::New(V8SafeCall<V8GetProc>));
}

void Library::V8Wrap(Library* library, v8::Persistent<v8::Object>& wrappedObj)
{
    v8::Handle<v8::Object> obj = V8Constructor->InstanceTemplate()->NewInstance();

    obj->SetInternalField(0, v8::External::New(library));

    v8::Persistent<v8::Object> persistentObj(v8::Isolate::GetCurrent(), obj);
    persistentObj.MakeWeak(library, V8WeakCallback);

    wrappedObj.Reset(v8::Isolate::GetCurrent(), persistentObj);
}

void Library::V8GetProc(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(args.Length() < 3)
    {
        throw std::runtime_error("Invalid argument count");
    }

    v8::Local<v8::Object> self = args.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    Library* library = (Library*)wrap->Value();

    v8::String::Utf8Value argProcName(args[0]);
    v8::String::Utf8Value argSignature(args[1]);
    v8::String::Utf8Value argCallingConventionStr(args[2]);
    Function::CallingConvention argCallingConvention;
    if(!strcmp(*argCallingConventionStr, "stdcall"))
    {
        argCallingConvention = Function::StdCall;
    }
    else if(!strcmp(*argCallingConventionStr, "cdecl"))
    {
        argCallingConvention = Function::CDecl;
    }
    else
    {
        throw std::runtime_error("Invalid calling convention");
    }

    Function* func = library->getProc(*argProcName, *argSignature, argCallingConvention);
    v8::Persistent<v8::Object> funcObj;
    Function::V8Wrap(func, funcObj);
    args.GetReturnValue().Set(funcObj);
}

void Library::V8WeakCallback(v8::Isolate* isolate, v8::Persistent<v8::Object>* object, Library* parameter)
{
    delete parameter;
 
    object->Dispose();
    object->Clear();
}
