#ifndef CALLBACKFUNCTION_H
#define CALLBACKFUNCTION_H

#include <string>
#include <v8.h>

class CallbackFunction
{
public:
    enum CallingConvention
    {
        StdCall, 
        CDecl,
    };

    CallbackFunction(const std::string& signature, CallingConvention callingConvention, const v8::Persistent<v8::Object>& callbackFunc);
    ~CallbackFunction();

    void setCallback(const v8::Persistent<v8::Object>& c);
    const v8::Persistent<v8::Object>& getCallback() const { return callback; }

    void* getAddress() const { return redirector; }

    const std::string& getSignature() const { return signature; }

    static void V8Init();
    static v8::Persistent<v8::FunctionTemplate> V8Constructor;

    static void V8ConstructorFunction(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void V8Wrap(CallbackFunction* library, v8::Persistent<v8::Object>& wrappedObj);
    static void V8WeakCallback(v8::Isolate* isolate, v8::Persistent<v8::Object>* object, CallbackFunction* parameter);

    static void V8GetAddress(const v8::FunctionCallbackInfo<v8::Value>& args);

private:
    void* redirector;
    std::string signature;
    CallingConvention callingConvention;
    v8::Persistent<v8::Object> callback;
};

#endif
