#ifndef ARRAYBUFFERUTILS_H
#define ARRAYBUFFERUTILS_H

#include <v8.h>
#include <stdlib.h>
#include <string.h>

class SimpleArrayBufferAllocator : public v8::ArrayBuffer::Allocator 
{
public:
    virtual void* Allocate(size_t length) 
    {
        void* result = malloc(length);
        memset(result, 0, length);
        return result;
    }

    virtual void* AllocateUninitialized(size_t length) 
    {
        return malloc(length);
    }

    virtual void Free(void* data, size_t) 
    { 
        free(data); 
    }

    static SimpleArrayBufferAllocator& getInstance()
    {
        static SimpleArrayBufferAllocator arrayBufferAllocator;
        return arrayBufferAllocator;
    }
};

inline void V8ExternalizeAutoDeleteWeakCallback(const v8::WeakCallbackData<v8::ArrayBuffer, v8::Persistent<v8::ArrayBuffer> >& data)
{
    // Delete contents of ArrayBuffer
    v8::Handle<v8::Value> contentsHeapValue = data.GetValue()->GetHiddenValue(v8::String::New("__jswin_external"));
    v8::Handle<v8::External> contentsHeapExternal = v8::Local<v8::External>::Cast(contentsHeapValue->ToObject());
    v8::ArrayBuffer::Contents* contentsHeap = static_cast<v8::ArrayBuffer::Contents*>(contentsHeapExternal->Value());
    SimpleArrayBufferAllocator::getInstance().Free(contentsHeap->Data(), contentsHeap->ByteLength());   // delete data
    delete contentsHeap;            // delete contents object
    data.GetParameter()->Reset();   // reset persistent handle
    delete data.GetParameter();     // delete persistent handle
}

inline void* ExternalizeAutoDelete(v8::Handle<v8::ArrayBuffer> arrayBuffer)
{
    void* ptr = NULL;

    // An ArrayBuffer can only be externalized once. Therefore, after externalizing it the first time, we store
    // a pointer to the contents as hidden value inside the ArrayBuffer. Furthermore, we make a weak handle,
    // deleting the contents inside the weak callback.

    if(!arrayBuffer->IsExternal())
    {
        // Externalize the buffer
        v8::ArrayBuffer::Contents contents = arrayBuffer->Externalize();
        ptr = contents.Data();
        
        // Store contents as hidden value
        v8::ArrayBuffer::Contents* contentsHeap = new v8::ArrayBuffer::Contents(contents);
        arrayBuffer->SetHiddenValue(v8::String::New("__jswin_external"), v8::External::New(contentsHeap));

        // Make weak handle
        v8::Persistent<v8::ArrayBuffer>* persistentArrayBuffer = new v8::Persistent<v8::ArrayBuffer>(v8::Isolate::GetCurrent(), arrayBuffer);
        persistentArrayBuffer->SetWeak<v8::ArrayBuffer, v8::Persistent<v8::ArrayBuffer> >(persistentArrayBuffer, V8ExternalizeAutoDeleteWeakCallback);
    }
    else
    {
        // Retrieve contents from hidden value
        v8::Handle<v8::Value> contentsHeapValue = arrayBuffer->GetHiddenValue(v8::String::New("__jswin_external"));
        v8::Handle<v8::External> contentsHeapExternal = v8::Local<v8::External>::Cast(contentsHeapValue->ToObject());
        v8::ArrayBuffer::Contents* contentsHeap = static_cast<v8::ArrayBuffer::Contents*>(contentsHeapExternal->Value());
        ptr = contentsHeap->Data();
    }
    return ptr;
}

#endif
