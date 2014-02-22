#include "Function.h"

#include <vector>

#include "V8SafeCall.h"
#include "V8ArrayBufferUtils.h"
#include "V8StringUtils.h"

v8::Persistent<v8::FunctionTemplate> Function::V8Constructor;

Function::Function(const std::string& signature, CallingConvention callingConvention, void* address)
    : signature(signature), callingConvention(callingConvention), address(address)
{
}

Function::~Function()
{
}

void Function::call(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    void* addressLocal = address;

    std::vector<char*> strings;
    std::vector<wchar_t*> wstrings;

    for(int i=signature.size()-2; i>=0; --i)
    {
        if(i > args.Length()-1)
        {
            throw std::runtime_error("Invalid argument count");
        }

        char type = signature[i+1];

        if(type == 'i')
        {
            int a = args[i]->Int32Value();
            _asm push a
        }
        else if(type == 'u')
        {
            int a = args[i]->Uint32Value();
            _asm push a
        }
        else if(type == 'c')
        {
            char* a = NULL;
			if(!args[i]->IsNull())
			{
                a = _strdup(ToAnsi(args[i]->ToString()).data());
                strings.push_back(a);
            }
            _asm push a
        }
        else if(type == 'w')
        {
            wchar_t* a = NULL;
			if(!args[i]->IsNull())
			{
                a = _wcsdup(ToWideChar(args[i]->ToString()).data());
                wstrings.push_back(a);
            }
            _asm push a
        }
        else if(type == 's')
        {
			void* a = NULL;
			if(!args[i]->IsNull())
			{
                if(!args[i]->IsArrayBuffer())
                {
                    throw std::runtime_error("Argument is not an array buffer");
                }
                v8::Handle<v8::ArrayBuffer> arrayBuffer = v8::Handle<v8::ArrayBuffer>::Cast(args[i]);

                a = ExternalizeAutoDelete(arrayBuffer);
            }
            _asm push a
        }
        else
        {
            throw std::runtime_error("Unknown argument type");
        }
    }
    
    unsigned int r = 0;
    _asm call dword ptr[addressLocal]
    _asm mov [r], eax

    if(callingConvention == CDecl)
    {
        // Clean up stack
        for(int i=signature.size()-2; i>=0; --i)
        {
            _asm pop eax
        }
    }

    for(std::vector<char*>::iterator it = strings.begin(); it != strings.end(); ++it)
    {
        free(*it);
    }
    for(std::vector<wchar_t*>::iterator it = wstrings.begin(); it != wstrings.end(); ++it)
    {
        free(*it);
    }

    if(signature[0] == 'u')
    {
        args.GetReturnValue().Set(static_cast<unsigned int>(r));
    }
    else if(signature[0] == 'i')
    {
        args.GetReturnValue().Set(static_cast<int>(r));
    }
    else if(signature[0] == 'v')
    {
        args.GetReturnValue().Set(v8::Undefined());
    }
    else
    {
        throw std::runtime_error("Unknown return type");
    }
}

void Function::V8Init()
{
    v8::Handle<v8::FunctionTemplate> V8ConstructorLocal = v8::FunctionTemplate::New();
    V8ConstructorLocal->InstanceTemplate()->SetInternalFieldCount(1);
    V8ConstructorLocal->InstanceTemplate()->SetCallAsFunctionHandler(V8Call);
    V8ConstructorLocal->PrototypeTemplate()->Set("getAddress", v8::FunctionTemplate::New(V8SafeCall<V8GetAddress>));
    V8Constructor.Reset(v8::Isolate::GetCurrent(), V8ConstructorLocal);
}

void Function::V8ConstructorFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(!args.IsConstructCall())
    {
        throw std::runtime_error("Constructor was called without 'new'");
    }

    if(args.Length() < 3)
    {
        throw std::runtime_error("Invalid argument count");
    }

    v8::String::Utf8Value argSignature(args[0]);
    v8::String::Utf8Value argCallingConventionStr(args[1]);
    void* argAddress = (void*)args[2]->Int32Value();
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

    Function* func = new Function(signature, argCallingConvention, argAddress);

    v8::Persistent<v8::Object> funcObj;
    V8Wrap(func, funcObj);
    
    args.GetReturnValue().Set(funcObj);
}

void Function::V8Wrap(Function* function, v8::Persistent<v8::Object>& wrappedObj)
{
    v8::Handle<v8::FunctionTemplate> V8ConstructorLocal = v8::Handle<v8::FunctionTemplate>::New(v8::Isolate::GetCurrent(), V8Constructor);
    v8::Handle<v8::Object> obj = V8ConstructorLocal->InstanceTemplate()->NewInstance();

    obj->SetInternalField(0, v8::External::New(function));

    v8::Persistent<v8::Object> persistentObj(v8::Isolate::GetCurrent(), obj);
    persistentObj.MakeWeak(function, V8WeakCallback);

    wrappedObj.Reset(v8::Isolate::GetCurrent(), persistentObj);
}

void Function::V8WeakCallback(v8::Isolate* isolate, v8::Persistent<v8::Object>* object, Function* parameter)
{
    delete parameter;
 
    object->Dispose();
    object->Clear();
}

void Function::V8Call(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Local<v8::Object> self = args.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    Function* function = (Function*)wrap->Value();
    function->call(args);
}

void Function::V8GetAddress(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Local<v8::Object> self = args.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    Function* func = (Function*)wrap->Value();
    void* address = func->getAddress();
    args.GetReturnValue().Set(v8::Uint32::New((unsigned int)address));
}
