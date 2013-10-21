#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <functional>

#include <v8.h>
#include <windows.h>

#include "CallbackFunction.h"
#include "Library.h"
#include "Function.h"
#include "V8SafeCall.h"
#include "V8ArrayBufferUtils.h"

std::string loadFile(const std::string& filename)
{
    std::ifstream file(filename);
    if(!file.is_open())
    {
        throw std::runtime_error("File not found");
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void V8Print(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    for(int i=0; i<args.Length(); ++i)
    {
        v8::String::Utf8Value str(args[i]);
        std::cout << *str;
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

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " FILENAME" << std::endl;
        return 0;
    }

    try
    {
        v8::Isolate* isolate = v8::Isolate::GetCurrent();

        v8::HandleScope handleScope(isolate);

        v8::V8::SetArrayBufferAllocator(&SimpleArrayBufferAllocator::getInstance());

        v8::Handle<v8::ObjectTemplate> globalObjectTemplate = v8::ObjectTemplate::New();

        globalObjectTemplate->Set("print", v8::FunctionTemplate::New(V8SafeCall<V8Print>));
        globalObjectTemplate->Set("loadLibrary", v8::FunctionTemplate::New(V8SafeCall<V8LoadLibrary>));
        globalObjectTemplate->Set("getBaseAddress", v8::FunctionTemplate::New(V8SafeCall<V8GetBaseAddress>));
        globalObjectTemplate->Set("exit", v8::FunctionTemplate::New(V8SafeCall<V8Exit>));

        Library::V8Init();
        Function::V8Init();
        CallbackFunction::V8Init();

        globalObjectTemplate->Set("DLLProc", v8::FunctionTemplate::New(V8SafeCall<Function::V8ConstructorFunction>));
        globalObjectTemplate->Set("CallbackFunction", v8::FunctionTemplate::New(V8SafeCall<CallbackFunction::V8ConstructorFunction>));

        v8::Handle<v8::Context> context = v8::Context::New(isolate, NULL, globalObjectTemplate);

        v8::Context::Scope contextScope(context);

        std::string sourceText = loadFile(argv[1]);
        v8::Handle<v8::String> source = v8::String::New(sourceText.c_str());
        v8::Handle<v8::Script> script = v8::Script::Compile(source, v8::String::New(argv[1]));
    
        v8::TryCatch tryCatch;
        v8::Handle<v8::Value> result = script->Run();
        if(tryCatch.HasCaught())
        {
            v8::String::Utf8Value errorMsg(tryCatch.Message()->Get());
            std::cout << *errorMsg << "\n";
        }

        int exitCode = 0;
        if(!result.IsEmpty())
        {
            exitCode = result->Int32Value();
        }
        return exitCode;
    }
    catch(const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return -1;
    }
}
