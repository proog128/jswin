#include "Require.h"

#include <boost/filesystem.hpp>
#include <iostream>

#include "V8SafeCall.h"
#include "GlobalFunctions.h"

namespace fs = boost::filesystem;

std::string loadFile(const ModuleContext& moduleContext, const std::string& filename)
{
    char* contents_c = moduleContext.readModule(moduleContext.data, filename.c_str());
    if(contents_c == NULL)
    {
        throw std::runtime_error("Module not found (" + filename + ")");
    }

    std::string contents(contents_c);
    jswin_free(contents_c);

    return contents;
}

bool beginsWith(const std::string& str, const std::string& begin)
{
    if(str.length() < begin.length())
        return false;

    return str.substr(0, begin.length()) == begin;
}

void V8Require(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if(args.Length() < 1)
    {
        throw std::runtime_error("Invalid number of arguments");
    }

    v8::Handle<v8::Object> returnValue;
    v8::Handle<v8::Value> exports = require(*v8::String::Utf8Value(args[0]->ToString()), args.Data()->ToObject(), returnValue);
    args.GetReturnValue().Set(exports);
}

v8::Local<v8::Object> initRequire(v8::Handle<v8::Context> context, ModuleContext& moduleContext, const std::string& moduleId)
{
    v8::Local<v8::Object> requireData = v8::Object::New();
    requireData->SetHiddenValue(v8::String::New("__MODULE_CONTEXT"), v8::External::New(&moduleContext));
    requireData->SetHiddenValue(v8::String::New("__MODULE_ID"), v8::String::New(moduleId.c_str()));
    context->Global()->Set(v8::String::New("require"), v8::FunctionTemplate::New(V8SafeCall<V8Require>, requireData)->GetFunction());
    return requireData;
}

fs::path makeCanonical(const fs::path& absolutePath)
{
    fs::path result;
    bool scan = true;
    while(scan)
    {
      scan = false;
      result.clear();
      for(fs::path::iterator it = absolutePath.begin(); it != absolutePath.end(); ++it)
      {
        if(*it == ".")
          continue;
        if(*it == "..")
        {
          result.remove_filename();
          continue;
        }

        result /= *it;
      }
    }
    return result;
}

v8::Handle<v8::Value> require(std::string moduleId, v8::Handle<v8::Object> requireData, v8::Handle<v8::Object>& returnValue)
{
    bool isRelative = beginsWith(moduleId, ".") || beginsWith(moduleId, "..");

    std::string currentModuleId = *v8::String::Utf8Value(requireData->GetHiddenValue(v8::String::New("__MODULE_ID")->ToString()));
    v8::Handle<v8::External> moduleContextExternal = v8::Local<v8::External>::Cast(requireData->GetHiddenValue(v8::String::New("__MODULE_CONTEXT")));
    ModuleContext* moduleContext = static_cast<ModuleContext*>(moduleContextExternal->Value());

    fs::path currentModulePath = fs::path(currentModuleId).parent_path();

    fs::path canonicalModuleId;
    if(isRelative)
    {
        canonicalModuleId = makeCanonical(currentModulePath / moduleId);
    }
    else
    {
        canonicalModuleId = makeCanonical(fs::path(moduleContext->rootDirectory) / moduleId);
    }

    std::string canonicalModuleIdString = canonicalModuleId.generic_string().c_str();

    ModuleContext::ModuleMapT& moduleList = moduleContext->modules;
    ModuleContext::ModuleMapT::const_iterator moduleIt = moduleList.find(canonicalModuleIdString);
    if(moduleIt != moduleList.end())
    {
        v8::HandleScope handleScope(v8::Isolate::GetCurrent());

        v8::Local<v8::Object> module = v8::Local<v8::Object>::New(v8::Isolate::GetCurrent(), moduleIt->second)->ToObject();
        v8::Local<v8::Value> exports = module->Get(v8::String::New("exports"));
        return handleScope.Close(exports);
    }
    else
    {
        v8::HandleScope handleScope(v8::Isolate::GetCurrent());

        v8::Handle<v8::ObjectTemplate> globalObjectTemplate = buildGlobalObject();

        v8::Local<v8::Context> Context = 
            v8::Context::New(v8::Isolate::GetCurrent(), NULL, globalObjectTemplate);
    
        v8::Context::Scope contextScope(Context);

        v8::Local<v8::Object> securityToken = v8::Local<v8::Object>::New(v8::Isolate::GetCurrent(), moduleContext->securityToken);
        Context->SetSecurityToken(securityToken);

        std::string canonicalModuleIdWithoutExtString = canonicalModuleId.replace_extension().string();

        v8::Local<v8::Object> module = v8::Object::New();
        v8::Local<v8::Object> exports = v8::Object::New();
        module->Set(v8::String::New("id"), v8::String::New(canonicalModuleIdWithoutExtString.c_str()));
        module->Set(v8::String::New("exports"), exports);

        Context->Global()->Set(v8::String::New("module"), module);
        Context->Global()->Set(v8::String::New("exports"), exports);

        initRequire(Context, *moduleContext, canonicalModuleIdWithoutExtString.c_str());

        moduleList.insert(std::make_pair(canonicalModuleIdString, ModuleContext::ModulePersistentT(v8::Isolate::GetCurrent(), module)));

        std::string sourceText = loadFile(*moduleContext, canonicalModuleIdString.c_str());
        v8::Handle<v8::String> source = v8::String::New(sourceText.c_str());

        v8::TryCatch tryCatch;
        v8::Handle<v8::Script> script = v8::Script::Compile(source, v8::String::New(canonicalModuleIdString.c_str()));
        if(tryCatch.HasCaught())
        {
            v8::String::Utf8Value errorMsg(tryCatch.Message()->Get());
            throw std::runtime_error(*errorMsg);
        }

        tryCatch.Reset();

        v8::Handle<v8::Value> result = script->Run();
        if(tryCatch.HasCaught())
        {
            v8::String::Utf8Value errorMsg(tryCatch.Message()->Get());
            throw std::runtime_error(*errorMsg);
        }

        return handleScope.Close(module->Get(v8::String::New("exports")));
    }
}