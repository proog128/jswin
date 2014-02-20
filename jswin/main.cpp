#include <iostream>

#include <v8.h>

#include "V8ArrayBufferUtils.h"
#include "GlobalFunctions.h"
#include "Require.h"
#include "Library.h"
#include "Function.h"
#include "CallbackFunction.h"

bool endsWith(const std::string& str, const std::string& end)
{
    if(str.length() < end.length())
        return false;

    return str.substr(str.length()-end.length(), end.length()) == end;
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
        std::string moduleId = argv[1];
        if(endsWith(moduleId, ".js"))
        {
            moduleId = moduleId.substr(0, moduleId.length() - 3);
        }

        ModuleContext moduleContext;

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
        moduleContext.securityToken.Reset(isolate, securityToken);
        context->SetSecurityToken(securityToken);

        v8::Handle<v8::Object> requireData = initRequire(context, moduleContext, "");
        v8::Handle<v8::Object> returnValue;
        require(moduleId, requireData, returnValue);

        int exitCode = 0;
        if(!returnValue.IsEmpty())
        {
            exitCode = returnValue->Int32Value();
        }
        return exitCode;
    }
    catch(const std::exception& ex)
    {
#ifdef _WINDOWS
        MessageBoxA(NULL, ex.what(), "Error", MB_OK | MB_ICONERROR);
#else
        std::cout << ex.what() << std::endl;
#endif
        return -1;
    }
}

#ifdef _WINDOWS
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif
