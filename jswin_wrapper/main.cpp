#include <iostream>
#include <numeric>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

bool endsWith(const std::string& str, const std::string& end)
{
    if(str.length() < end.length())
        return false;

    return str.substr(str.length()-end.length(), end.length()) == end;
}

struct Module
{
    std::string id;
    std::string filename;
    std::streamoff pos;
    std::streamsize size;
};

std::vector<Module> listModules(const std::string& mainModuleFilename)
{
    std::vector<Module> modules;
    fs::path mainModuleCanonical = fs::canonical(mainModuleFilename);
    fs::path modulePath = mainModuleCanonical.parent_path();

    // add main module (must be first one)
    Module mainModule;
    mainModule.id = std::string("/").append(mainModuleCanonical.filename().replace_extension("").generic_string());
    mainModule.filename = mainModuleCanonical.string();
    modules.push_back(mainModule);

    for(fs::recursive_directory_iterator end, it(modulePath);  it != end; ++it)
    {
        fs::path file(it->path());
        if(file != mainModuleCanonical && file.extension() == ".js")
        {
            // make relative to module path
            std::pair<fs::path::iterator, fs::path::iterator> diff = 
                std::mismatch(file.begin(), file.end(), modulePath.begin());
            fs::path relative = std::accumulate(diff.first, file.end(), fs::path(), std::divides<fs::path>());

            // remove extension
            Module module;
            module.id = std::string("/").append(relative.replace_extension("").generic_string());
            module.filename = file.string();
            modules.push_back(module);
        }
    }
    return modules;
}

void run(const std::string& mainModuleId, const std::string& loader, const std::string& exeName)
{
    std::vector<Module> modules = listModules(mainModuleId);

    std::ofstream exe(exeName, std::ios_base::binary);

    // copy loader
    {
        std::ifstream loaderExe(loader, std::ios_base::binary);
        if(!loaderExe.is_open())
        {
            throw std::exception(std::string("Could not find file ").append(loader).c_str());
        }
        exe << loaderExe.rdbuf();
    }

    // copy scripts
    for(std::vector<Module>::iterator it = modules.begin(); it != modules.end(); ++it)
    {
        it->pos = exe.tellp();

        std::ifstream script(it->filename, std::ios_base::binary);
        if(!script.is_open())
        {
            throw std::exception(std::string("Could not find file ").append(it->filename).c_str());
        }
        exe << script.rdbuf();
        exe.clear();

        it->size = exe.tellp() - it->pos;
    }

    // write index
    long version = 1;
    long indexPos = static_cast<long>(exe.tellp());
    long moduleCount = modules.size();
    exe.write(reinterpret_cast<const char*>(&moduleCount), 4);
    for(std::vector<Module>::iterator it = modules.begin(); it != modules.end(); ++it)
    {
        long pos = static_cast<long>(it->pos);
        long scriptSize = static_cast<long>(it->size);
        long idSize = static_cast<long>(it->id.size());
        exe.write(reinterpret_cast<const char*>(&pos), 4);
        exe.write(reinterpret_cast<const char*>(&scriptSize), 4);
        exe.write(reinterpret_cast<const char*>(&idSize), 4);
        exe << it->id;
    }
    exe.write(reinterpret_cast<const char*>(&indexPos), 4);
    exe.write(reinterpret_cast<const char*>(&version), 4);
    exe << "JW";

    exe.close();
}

int main(int argc, char* argv[])
{
    std::string moduleId;
    std::string loader;
    std::string exeName;

    po::options_description generic("Options");
    generic.add_options()
        ("help,h", "show help message")
        ("loader,l", po::value<std::string>(&loader)->default_value("jswin_loader.exe"), "loader application")
        ;
    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("js-module", po::value<std::string>(&moduleId), "main module")
        ("exe-name", po::value<std::string>(&exeName), "executable name")
        ;

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(hidden);

    po::positional_options_description p;
    p.add("js-module", 1);
    p.add("exe-name", -1);

    po::variables_map vm;
    try
    {
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
        po::notify(vm);
    }
    catch(const boost::program_options::error& e)
    {
        std::wcerr << e.what() << "\n";
        return -1;
    }

    if(vm.count("help") || moduleId.empty() || exeName.empty())
    {
        std::cout << "Usage: " << argv[0] << " [Options] JS-MODULE EXE-NAME\nWraps all modules found in the directory tree starting at JS-MODULE in executable EXE_NAME.\n\n";
        std::cout << generic << std::endl;
        return 0;
    }

    if(!endsWith(moduleId, ".js"))
    {
        moduleId += ".js";
    }

    try
    {
        run(moduleId, loader, exeName);
    }
    catch(const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return -1;
    }
}
