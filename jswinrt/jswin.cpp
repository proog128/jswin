#include "jswin.h"

#define NOMINMAX
#include <memory>

#include <v8.h>

#include "V8ArrayBufferUtils.h"
#include "GlobalFunctions.h"
#include "Require.h"
#include "Library.h"
#include "Function.h"
#include "CallbackFunction.h"

struct jswin_ctx
{
    ModuleContext moduleContext;
    v8::Persistent<v8::Context> context;
    v8::Persistent<v8::Object> requireData;
    
    ErrorCodes errorCode;
    std::string errorMessage;
};

jswin_ctx* jswin_init()
{
    try
    {
        std::unique_ptr<jswin_ctx> jswinContext(new jswin_ctx);
        jswinContext->errorCode = SUCCESS;

        v8::Isolate* isolate = v8::Isolate::GetCurrent();

        v8::HandleScope handleScope(isolate);

        v8::V8::SetArrayBufferAllocator(&SimpleArrayBufferAllocator::getInstance());

        Library::V8Init();
        Function::V8Init();
        CallbackFunction::V8Init();

        v8::Handle<v8::ObjectTemplate> globalObjectTemplate = buildGlobalObject();

        v8::Handle<v8::Context> context = v8::Context::New(isolate, NULL, globalObjectTemplate);
        v8::Context::Scope contextScope(context);

        v8::Handle<v8::Object> securityToken = v8::Object::New();
        jswinContext->moduleContext.securityToken.Reset(isolate, securityToken);
        context->SetSecurityToken(securityToken);

        v8::Handle<v8::Object> requireData = initRequire(context, jswinContext->moduleContext, "");

        jswinContext->context.Reset(isolate, context);
        jswinContext->requireData.Reset(isolate, requireData);

        return jswinContext.release();
    }
    catch(const std::exception&)
    {
        return NULL;
    }
}

void jswin_shutdown(jswin_ctx* ctx)
{
    try
    {
        v8::Isolate* isolate = v8::Isolate::GetCurrent();

        v8::HandleScope handleScope(isolate);

        {
            v8::Context::Scope contextScope(v8::Handle<v8::Context>::New(isolate, ctx->context));
            ctx->requireData.Reset();
        }
        ctx->context.Reset();
    }
    catch(const std::exception&)
    {
        // ...
    }

    delete ctx;
}

int jswin_get_error_msg(jswin_ctx* ctx, char* message, int max_length)
{
    if(message == NULL)
    {
        return ctx->errorMessage.size() + 1;
    }
    else
    {
        int size = std::min(max_length-1, static_cast<int>(ctx->errorMessage.size()));
        strncpy(message, ctx->errorMessage.c_str(), size);
        message[size] = 0;
        return size;
    }
}

int jswin_run_script(jswin_ctx* ctx, const char* module_id, int* exitCode)
{
    try
    {
        v8::Isolate* isolate = v8::Isolate::GetCurrent();

        v8::HandleScope handleScope(isolate);

        v8::Context::Scope contextScope(v8::Handle<v8::Context>::New(isolate, ctx->context));

        v8::Handle<v8::Object> returnValue;
        require(module_id, v8::Handle<v8::Object>::New(isolate, ctx->requireData), returnValue);

        if(!returnValue.IsEmpty())
        {
            *exitCode = returnValue->Int32Value();
        }

        return SUCCESS;
    }
    catch(const std::exception& ex)
    {
        ctx->errorCode = GENERAL_ERROR;
        ctx->errorMessage = ex.what();
        
        return GENERAL_ERROR;
    }
}
