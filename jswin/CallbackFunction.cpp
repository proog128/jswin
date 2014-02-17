#include "callbackFunction.h"
#include <vector>
#include <iostream>

#include "V8SafeCall.h"

#include <windows.h>

v8::Handle<v8::FunctionTemplate> CallbackFunction::V8Constructor;

unsigned int __stdcall genericCallbackFunction(CallbackFunction* cb)
{
    v8::HandleScope handleScope(v8::Isolate::GetCurrent());

    v8::Handle<v8::Object> func = v8::Local<v8::Object>::New(v8::Isolate::GetCurrent(), cb->getCallback());

    // Convert arguments on stack to JS based on signature definition
    const std::string& signature = cb->getSignature();

    std::vector<v8::Handle<v8::Value> > args;
    for(size_t i=1; i<signature.length(); ++i)
    {
        if(signature[i] == 'i')
        {
            unsigned int v = 0;
            unsigned int o = 16 + i*4;
            _asm mov eax, ebp
            _asm add eax, o
            _asm mov ecx, [eax]
            _asm mov [v], ecx
            args.push_back(v8::Int32::New(v));
        }
        else if(signature[i] == 'u')
        {
            int v = 0;
            unsigned int o = 16 + i*4;
            _asm mov eax, ebp
            _asm add eax, o
            _asm mov ecx, [eax]
            _asm mov [v], ecx
            args.push_back(v8::Uint32::New(v));
        }
        else
        {
            throw std::runtime_error("Unknown argument type");
        }
    }

    v8::Handle<v8::Value> returnObj = func->CallAsFunction(v8::Object::New(), args.size(), args.data());

    if(signature[0] == 'i')
    {
        return returnObj->Uint32Value();
    }
    else if(signature[0] == 'u')
    {
        return returnObj->Uint32Value();
    }
    else if(signature[0] == 'v')
    {
        return 0;
    }
    else
    {
        throw std::runtime_error("Unknown return type");
    }
}

static const unsigned char FUNC_CODE[] = {
    // _asm push ababababh
    // _asm mov eax, ebebebebh
    // _asm call eax
    // _asm ret ddddh
    0x68, 0xab, 0xab, 0xab, 0xab, 0xb8, 0xeb, 0xeb, 0xeb, 0xeb, 0xff, 0xd0, 0xc2, 0xdd, 0xdd
};

CallbackFunction::CallbackFunction(const std::string& signature, CallingConvention callingConvention, const v8::Persistent<v8::Object>& callbackFunc)
    : signature(signature), callingConvention(callingConvention)
{
    callback.Reset(v8::Isolate::GetCurrent(), callbackFunc);

    unsigned int argLength = 0;

    if(callingConvention == StdCall)
    {
        // We have to clean the stack
        argLength = (signature.length()-1) * sizeof(int);
    }
    else if(callingConvention == CDecl)
    {
        // Caller cleans the stack
        argLength = 0;
    }

    // Allocate memory that can be executed. TODO: Don't allocate a complete page.
    unsigned char* code = (unsigned char*)VirtualAlloc(NULL, sizeof(FUNC_CODE), MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    VirtualAlloc(code, sizeof(FUNC_CODE), MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    //unsigned char* code = new unsigned char[sizeof(FUNC_CODE)];
    memcpy(code, FUNC_CODE, sizeof(FUNC_CODE));

    unsigned int callbackInfoObjAddress = (unsigned int)this;
    unsigned int testFuncAddress = (unsigned int)&genericCallbackFunction;
    code[1] = callbackInfoObjAddress;
    code[2] = (callbackInfoObjAddress & 0xff00) >> 8;
    code[3] = (callbackInfoObjAddress & 0xff0000) >> 16;
    code[4] = (callbackInfoObjAddress & 0xff000000) >> 24;
    code[6] = testFuncAddress;
    code[7] = (testFuncAddress & 0xff00) >> 8;
    code[8] = (testFuncAddress & 0xff0000) >> 16;
    code[9] = (testFuncAddress & 0xff000000) >> 24;
    code[13] = argLength;
    code[14] = (argLength & 0xff00) >> 8;

    redirector = code;
}

CallbackFunction::~CallbackFunction()
{
}

void CallbackFunction::setCallback(const v8::Persistent<v8::Object>& c)
{
    callback.Reset(v8::Isolate::GetCurrent(), c);
}

void CallbackFunction::V8Init()
{
    V8Constructor = v8::FunctionTemplate::New();
    V8Constructor->InstanceTemplate()->SetInternalFieldCount(1);
    V8Constructor->PrototypeTemplate()->Set("getAddress", v8::FunctionTemplate::New(V8SafeCall<V8GetAddress>));
}

void CallbackFunction::V8ConstructorFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(!args.IsConstructCall())
    {
        throw std::runtime_error("Constructor was called without 'new'");
    }

    if(args.Length() < 2)
    {
        throw std::runtime_error("Invalid argument count");
    }

    v8::String::Utf8Value argSignature(args[0]);

    v8::String::Utf8Value argCallingConventionStr(args[1]);
    CallbackFunction::CallingConvention argCallingConvention;
    if(!strcmp(*argCallingConventionStr, "stdcall"))
    {
        argCallingConvention = CallbackFunction::StdCall;
    }
    else if(!strcmp(*argCallingConventionStr, "cdecl"))
    {
        argCallingConvention = CallbackFunction::CDecl;
    }
    else
    {
        throw std::runtime_error("Invalid calling convention");
    }

    std::string signature = "i";    // Set 'i' as default for now (compatibility)
    signature += *argSignature;
    if(args.Length() > 3)
    {
        v8::String::Utf8Value argReturnType(args[3]);
        if(argReturnType.length() != 1)
        {
            throw std::runtime_error("Invalid length of return type specifier");
        }

        signature[0] = (*argReturnType)[0];
    }

    v8::Handle<v8::Object> argCallbackObj = args[2]->ToObject();
    v8::Persistent<v8::Object> argCallbackPersistentObj(v8::Isolate::GetCurrent(), argCallbackObj);

    CallbackFunction* cbFunc = new CallbackFunction(signature, argCallingConvention, argCallbackPersistentObj);

    v8::Persistent<v8::Object> cbFuncObj;
    V8Wrap(cbFunc, cbFuncObj);
    
    args.GetReturnValue().Set(cbFuncObj);
}

void CallbackFunction::V8Wrap(CallbackFunction* library, v8::Persistent<v8::Object>& wrappedObj)
{
    v8::Handle<v8::Object> obj = V8Constructor->InstanceTemplate()->NewInstance();

    obj->SetInternalField(0, v8::External::New(library));

    v8::Persistent<v8::Object> persistentObj(v8::Isolate::GetCurrent(), obj);
    persistentObj.MakeWeak(library, V8WeakCallback);

    wrappedObj.Reset(v8::Isolate::GetCurrent(), persistentObj);
}

void CallbackFunction::V8WeakCallback(v8::Isolate* isolate, v8::Persistent<v8::Object>* object, CallbackFunction* parameter)
{
    delete parameter;
 
    object->Dispose();
    object->Clear();
}

void CallbackFunction::V8GetAddress(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Local<v8::Object> self = args.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    CallbackFunction* cbFunc = (CallbackFunction*)wrap->Value();
    void* address = cbFunc->getAddress();
    args.GetReturnValue().Set(v8::Uint32::New((unsigned int)address));
}
