#ifndef REQUIRE_H
#define REQUIRE_H

#include <v8.h>
#include <map>

#include "jswin.h"

struct ModuleContext
{
    typedef v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> > ModulePersistentT;
    typedef std::map<std::string, ModulePersistentT> ModuleMapT;
    ModuleMapT modules;
    v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object> > securityToken; // CopyablePersistentTraits for auto-delete of persistent
    std::string rootDirectory;
    read_module_func readModule;
    void* data;
};

v8::Local<v8::Object> initRequire(v8::Handle<v8::Context> context, ModuleContext& moduleContext, const std::string& moduleId);
v8::Handle<v8::Value> require(std::string moduleId, v8::Handle<v8::Object> requireData, v8::Handle<v8::Object>& returnValue);

#endif
