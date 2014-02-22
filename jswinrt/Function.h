#ifndef FUNCTION_H
#define FUNCTION_H

#include <string>
#include <v8.h>

class Function
{
public:
    enum CallingConvention
    {
        StdCall, 
        CDecl,
    };

    Function(const std::string& signature, CallingConvention callingConvention, void* address);
    ~Function();

    void call(const v8::FunctionCallbackInfo<v8::Value>& args);
    void* getAddress() const { return address; }

    static void V8Init();
    static v8::Persistent<v8::FunctionTemplate> V8Constructor;

    static void V8ConstructorFunction(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void V8Wrap(Function* function, v8::Persistent<v8::Object>& wrappedObj);
    static void V8WeakCallback(v8::Isolate* isolate, v8::Persistent<v8::Object>* object, Function* parameter);
    
    static void V8Call(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void V8GetAddress(const v8::FunctionCallbackInfo<v8::Value>& args);

private:
    std::string signature;
    CallingConvention callingConvention;
    void* address;
};

#endif
