#include <iostream>
#include <string>
#include <windows.h>

#include "jswin.h"

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

    std::string moduleId = argv[1];
    if(endsWith(moduleId, ".js"))
    {
        moduleId = moduleId.substr(0, moduleId.length() - 3);
    }

    jswin_ctx* ctx = jswin_init();

    int exitCode;
    
    if(JSWIN_FAILED(jswin_run_script(ctx, moduleId.c_str(), &exitCode)))
    {
        exitCode = -1;

        int length = jswin_get_error_msg(ctx, 0, 0);
        char* message = new char[length];
        jswin_get_error_msg(ctx, message, length);
#ifdef _WINDOWS
        MessageBoxA(NULL, message, "Error", MB_OK | MB_ICONERROR);
#else
        std::cout << message << std::endl;
#endif
    }

    jswin_shutdown(ctx);

    return exitCode;
}

#ifdef _WINDOWS
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif
