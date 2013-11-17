#include "Require.h"

#include <boost/filesystem.hpp>
#include <iostream>

#include "V8SafeCall.h"
#include "GlobalFunctions.h"

std::string loadFile(const std::string& filename)
{
    std::ifstream file(filename);
    if(!file.is_open())
    {
        throw std::runtime_error("File not found");
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
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

v8::Handle<v8::Value> require(std::string moduleId, v8::Handle<v8::Object> requireData, v8::Handle<v8::Object>& returnValue)
{
    bool isRelative = beginsWith(moduleId, ".") || beginsWith(moduleId, "..");
    moduleId += ".js";

    std::string currentModuleId = *v8::String::Utf8Value(requireData->GetHiddenValue(v8::String::New("__MODULE_ID")->ToString()));
    v8::Handle<v8::External> moduleContextExternal = v8::Local<v8::External>::Cast(requireData->GetHiddenValue(v8::String::New("__MODULE_CONTEXT")));
    ModuleContext* moduleContext = static_cast<ModuleContext*>(moduleContextExternal->Value());

    boost::filesystem::path currentModulePath = boost::filesystem::path(currentModuleId).parent_path();

    boost::filesystem::path canonicalModuleId;
    if(isRelative)
    {
        canonicalModuleId = boost::filesystem::canonical(currentModulePath / moduleId);
    }
    else
    {
        canonicalModuleId = boost::filesystem::canonical(moduleId);
    }

    std::string canonicalModuleIdString = canonicalModuleId.string().c_str();

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
        
        std::string canonicalModuleIdWithoutExtString = canonicalModuleId.replace_extension().string();

        v8::Local<v8::Object> module = v8::Object::New();
        v8::Local<v8::Object> exports = v8::Object::New();
        module->Set(v8::String::New("id"), v8::String::New(canonicalModuleIdWithoutExtString.c_str()));
        module->Set(v8::String::New("exports"), exports);

        Context->Global()->Set(v8::String::New("module"), module);
        Context->Global()->Set(v8::String::New("exports"), exports);

        initRequire(Context, *moduleContext, canonicalModuleIdWithoutExtString.c_str());

        moduleList.insert(std::make_pair(canonicalModuleIdString, ModuleContext::ModulePersistentT(v8::Isolate::GetCurrent(), module)));

        std::string sourceText = loadFile((canonicalModuleIdString).c_str());
        v8::Handle<v8::String> source = v8::String::New(sourceText.c_str());
        v8::Handle<v8::Script> script = v8::Script::Compile(source, v8::String::New(canonicalModuleIdString.c_str()));
    
        v8::TryCatch tryCatch;
        v8::Handle<v8::Value> result = script->Run();
        if(tryCatch.HasCaught())
        {
            v8::String::Utf8Value errorMsg(tryCatch.Message()->Get());
            std::cout << *errorMsg << "\n";
        }

        return handleScope.Close(module->Get(v8::String::New("exports")));
    }
}