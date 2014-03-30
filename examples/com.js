// Copies a file using IFileOperation::CopyItem.

var sourceFile = "C:\\Temp\\a\\source.txt";
var destDir = "C:\\Temp\\b";
var destFilename = "dest.txt";

var ole32 = loadLibrary("ole32.dll");
var CoInitializeEx = ole32.getProc("CoInitializeEx", "ii", "stdcall");
var CoCreateInstance = ole32.getProc("CoCreateInstance", "siiss", "stdcall");
var CoTaskMemFree = ole32.getProc("CoTaskMemFree", "i", "stdcall");

var shell32 = loadLibrary("shell32.dll");
var SHCreateItemFromParsingName = shell32.getProc("SHCreateItemFromParsingName", "wiss", "stdcall");

var COINITBASE_MULTITHREADED  = 0x0;
var COINIT_APARTMENTTHREADED  = 0x2;
var COINIT_MULTITHREADED      = COINITBASE_MULTITHREADED;
var COINIT_DISABLE_OLE1DDE    = 0x4;
var COINIT_SPEED_OVER_MEMORY  = 0x8;

var S_OK    = 0;
var S_FALSE = 1;

var CLSCTX_INPROC_SERVER    = 0x1;
var CLSCTX_INPROC_HANDLER   = 0x2;
var CLSCTX_LOCAL_SERVER = 0x4;
var CLSCTX_INPROC_SERVER16  = 0x8;
var CLSCTX_REMOTE_SERVER    = 0x10;
var CLSCTX_INPROC_HANDLER16 = 0x20;
var CLSCTX_RESERVED1    = 0x40;
var CLSCTX_RESERVED2    = 0x80;
var CLSCTX_RESERVED3    = 0x100;
var CLSCTX_RESERVED4    = 0x200;
var CLSCTX_NO_CODE_DOWNLOAD = 0x400;
var CLSCTX_RESERVED5    = 0x800;
var CLSCTX_NO_CUSTOM_MARSHAL    = 0x1000;
var CLSCTX_ENABLE_CODE_DOWNLOAD = 0x2000;
var CLSCTX_NO_FAILURE_LOG   = 0x4000;
var CLSCTX_DISABLE_AAA  = 0x8000;
var CLSCTX_ENABLE_AAA   = 0x10000;
var CLSCTX_FROM_DEFAULT_CONTEXT = 0x20000;
var CLSCTX_ACTIVATE_32_BIT_SERVER   = 0x40000;
var CLSCTX_ACTIVATE_64_BIT_SERVER   = 0x80000;
var CLSCTX_ENABLE_CLOAKING  = 0x100000;
var CLSCTX_APPCONTAINER = 0x400000;
var CLSCTX_ACTIVATE_AAA_AS_IU   = 0x800000;
var CLSCTX_PS_DLL   = 0x80000000;
var CLSCTX_INPROC = CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER;
var CLSCTX_ALL = CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER;
var CLSCTX_SERVER = CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER;

function failed(hresult) {
    return hresult != S_OK;
}

if(failed(CoInitializeEx(0, COINITBASE_MULTITHREADED))) {
    print("CoInitializeEx failed");
    exit(-1);
}

var CLSID_FileOperation = new DataView(new ArrayBuffer(16));
CLSID_FileOperation.setUint32(0, 0x3ad05575, true);
CLSID_FileOperation.setUint16(4, 0x8857, true);
CLSID_FileOperation.setUint16(6, 0x4850, true);
CLSID_FileOperation.setUint16(8, 0x9277, false);
CLSID_FileOperation.setUint8(10, 0x11);
CLSID_FileOperation.setUint8(11, 0xb8);
CLSID_FileOperation.setUint8(12, 0x5b);
CLSID_FileOperation.setUint8(13, 0xdb);
CLSID_FileOperation.setUint8(14, 0x8e);
CLSID_FileOperation.setUint8(15, 0x09);

var IID_IFileOperation = new DataView(new ArrayBuffer(16));
IID_IFileOperation.setUint32(0, 0x947aab5f, true);
IID_IFileOperation.setUint16(4, 0x0a5c, true);
IID_IFileOperation.setUint16(6, 0x4c13, true);
IID_IFileOperation.setUint16(8, 0xb4d6, false);
IID_IFileOperation.setUint8(10, 0x4b);
IID_IFileOperation.setUint8(11, 0xf7);
IID_IFileOperation.setUint8(12, 0x83);
IID_IFileOperation.setUint8(13, 0x6f);
IID_IFileOperation.setUint8(14, 0xc9);
IID_IFileOperation.setUint8(15, 0xf8);

var ppFileOperation = new DataView(new ArrayBuffer(4));
if(failed(CoCreateInstance(CLSID_FileOperation.buffer, 0, CLSCTX_INPROC_SERVER, IID_IFileOperation.buffer, ppFileOperation.buffer))) {
    print("CoCreateInstance failed");
    exit(-1);
}

var pFileOperation = new DataView(fromMemory(ppFileOperation.getUint32(0, true), 32));
var IFileOperation_vtbl = new DataView(fromMemory(pFileOperation.getUint32(0, true), 92));
var IFileOperation_QueryInterface = new DLLProc("sss", "stdcall", IFileOperation_vtbl.getUint32(0, true));
var IFileOperation_AddRef = new DLLProc("s", "stdcall", IFileOperation_vtbl.getUint32(4, true));
var IFileOperation_Release = new DLLProc("s", "stdcall", IFileOperation_vtbl.getUint32(8, true));
var IFileOperation_CopyItem = new DLLProc("sssws", "stdcall", IFileOperation_vtbl.getUint32(64, true));
var IFileOperation_PerformOperations = new DLLProc("s", "stdcall", IFileOperation_vtbl.getUint32(84, true));

var IID_IShellItem = new DataView(new ArrayBuffer(16));
IID_IShellItem.setUint32(0, 0x43826d1e, true);
IID_IShellItem.setUint16(4, 0xe718, true);
IID_IShellItem.setUint16(6, 0x42ee, true);
IID_IShellItem.setUint16(8, 0xbc55, false);
IID_IShellItem.setUint8(10, 0xa1);
IID_IShellItem.setUint8(11, 0xe2);
IID_IShellItem.setUint8(12, 0x61);
IID_IShellItem.setUint8(13, 0xc3);
IID_IShellItem.setUint8(14, 0x7b);
IID_IShellItem.setUint8(15, 0xfe);

var ppSourceFile = new DataView(new ArrayBuffer(4));
if(failed(SHCreateItemFromParsingName(sourceFile, 0, IID_IShellItem.buffer, ppSourceFile.buffer))) {
    print("SHCreateItemFromParsingName failed");
    exit(-1);
}

var pSourceFile = new DataView(fromMemory(ppSourceFile.getUint32(0, true), 32));
var IShellItem_vtbl = new DataView(fromMemory(pSourceFile.getUint32(0, true), 32));

var IShellItem_QueryInterface = new DLLProc("sss", "stdcall", IShellItem_vtbl.getUint32(0, true));
var IShellItem_AddRef = new DLLProc("s", "stdcall", IShellItem_vtbl.getUint32(4, true));
var IShellItem_Release = new DLLProc("s", "stdcall", IShellItem_vtbl.getUint32(8, true));
var IShellItem_GetDisplayName = new DLLProc("sis", "stdcall", IShellItem_vtbl.getUint32(20, true));

var pName = new DataView(new ArrayBuffer(4));
IShellItem_GetDisplayName(pSourceFile.buffer, 0, pName.buffer);
CoTaskMemFree(pName.getUint32(0, true));

var ppDestinationDir = new DataView(new ArrayBuffer(4));
if(failed(SHCreateItemFromParsingName(destDir, 0, IID_IShellItem.buffer, ppDestinationDir.buffer))) {
    print("SHCreateItemFromParsingName failed");
    exit(-1);
}
var pDestinationDir = new DataView(fromMemory(ppDestinationDir.getUint32(0, true), 32));
// don't need to retrieve vtable again because it is the same for each object of type IShellItem

IFileOperation_CopyItem(pFileOperation.buffer, pSourceFile.buffer, pDestinationDir.buffer, destFilename, null);
IFileOperation_PerformOperations(pFileOperation.buffer);

IShellItem_Release(pSourceFile.buffer);
IShellItem_Release(pDestinationDir.buffer);

IFileOperation_Release(pFileOperation.buffer);
