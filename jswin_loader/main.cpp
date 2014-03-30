#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <windows.h>

#include "jswin.h"

bool endsWith(const std::string& str, const std::string& end)
{
    if(str.length() < end.length())
        return false;

    return str.substr(str.length()-end.length(), end.length()) == end;
}

std::string exeName()
{
    char name[1024];
    GetModuleFileNameA(NULL, name, 1024);
    return name;
}

struct Module
{
    std::streamoff pos;
    std::streamsize size;
};

std::pair<std::string, std::map<std::string, Module> > readIndex(std::ifstream& exe)
{
    std::map<std::string, Module> modules;

    // read magic
    exe.seekg(-2, std::ios_base::end);
    char magic[2];
    exe.read(magic, 2);
    if(magic[0] != 'J' && magic[1] != 'W')
    {
        std::cout << "Could not find index.\n";
        throw std::exception("Could not find index");
    }

    // read index
    exe.seekg(-10, std::ios_base::end);
    long indexPos = 0;
    long version = 0;
    exe.read(reinterpret_cast<char*>(&indexPos), 4);
    exe.read(reinterpret_cast<char*>(&version), 4);
    if(version != 1)
    {
        throw std::exception("Unsupported version");
    }
    exe.seekg(indexPos, std::ios_base::beg);

    long moduleCount = 0;
    exe.read(reinterpret_cast<char*>(&moduleCount), 4);
    std::string rootModuleId;
    for(long i=0; i<moduleCount; ++i)
    {
        long pos = 0;
        long scriptSize = 0;
        long idSize = 0;
        exe.read(reinterpret_cast<char*>(&pos), 4);
        exe.read(reinterpret_cast<char*>(&scriptSize), 4);
        exe.read(reinterpret_cast<char*>(&idSize), 4);
        std::string moduleId;
        moduleId.resize(idSize);
        exe.read(&moduleId[0], idSize);

        Module m;
        m.pos = pos;
        m.size = scriptSize;
        modules.insert(std::make_pair(moduleId, m));

        if(i == 0)
        {
            rootModuleId = moduleId.substr(1);  // Remove leading '/'
        }
    }
    return std::make_pair(rootModuleId, modules);
}

struct LoaderContext
{
    std::map<std::string, Module> index;
    std::ifstream exe;
};

char* readModule(void* data, const char* module_id)
{
    LoaderContext* lctx = static_cast<LoaderContext*>(data);
    std::map<std::string, Module>::const_iterator it = lctx->index.find(module_id);
    if(it == lctx->index.end())
    {
        return NULL;
    }

    lctx->exe.seekg(it->second.pos);
    
    char* script = static_cast<char*>(jswin_alloc(it->second.size+1));
    lctx->exe.read(script, it->second.size);
    script[it->second.size] = 0;
    return script;
}

int main(int argc, char* argv[])
{
    LoaderContext lctx;
    std::string exename = exeName();
    lctx.exe.open(exename, std::ios_base::binary);
    if(!lctx.exe.is_open())
    {
        std::cout << "Could not open " << exename << " for reading.\n";
        return -1;
    }

    int exitCode = -1;

    try
    {
        std::string moduleId;

        std::pair<std::string, std::map<std::string, Module> > index = readIndex(lctx.exe);
        moduleId = index.first;
        lctx.index = index.second;

        jswin_ctx* ctx = jswin_init("/", readModule, &lctx);
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
    }
    catch(const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return -1;
    }

    return exitCode;
}

#ifdef _WINDOWS
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif
