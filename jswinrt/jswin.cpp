#include "jswin.h"

#define NOMINMAX
#include <memory>
#include <string>
#include <streambuf>

#include <boost/filesystem.hpp>

#include <v8.h>

#include "V8ArrayBufferUtils.h"
#include "GlobalFunctions.h"
#include "Require.h"
#include "Library.h"
#include "Function.h"
#include "CallbackFunction.h"

namespace fs = boost::filesystem;

struct jswin_ctx
{
    ModuleContext moduleContext;
    v8::Persistent<v8::Context> context;
    v8::Persistent<v8::Object> requireData;
    
    ErrorCodes errorCode;
    std::string errorMessage;
};

char* default_read_module(void* data, const char* module_id)
{
    std::string moduleIdWithExt(module_id);
    moduleIdWithExt += ".js";
    std::ifstream file(moduleIdWithExt);
    if(!file.is_open())
    {
        return NULL;
    }

    std::istreambuf_iterator<char> it(file);
    std::string contents(it, std::istreambuf_iterator<char>());
    size_t s = contents.size();
    char* contents_c = static_cast<char*>(jswin_alloc(s+1));
    stdext::checked_array_iterator<char*> contents_c_chk(contents_c, s+1);
    std::copy(contents.begin(), contents.end(), contents_c_chk);
    contents_c_chk[s] = 0;
    return contents_c;
}

jswin_ctx* jswin_init(const char* root_directory, read_module_func read_module, void* data)
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

        jswinContext->moduleContext.rootDirectory = root_directory ? root_directory : fs::current_path().string();
        jswinContext->moduleContext.readModule = read_module ? read_module : default_read_module;
        jswinContext->moduleContext.data = data;

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
        else
        {
            *exitCode = 0;
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

int jswin_register_read_module_callback(jswin_ctx* ctx, void* data, read_module_func read_module)
{
    return SUCCESS;
}

JSWIN_API void* jswin_alloc(size_t size)
{
    return malloc(size);
}

JSWIN_API void jswin_free(void* p)
{
    free(p);
}

