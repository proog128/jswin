function ab2str(buf) {
  return String.fromCharCode.apply(null, new Uint8Array(buf));
}

function str2ab(str) {
  var buf = new ArrayBuffer(str.length + 1);
  var bufView = new Uint8Array(buf);
  for (var i=0, strLen=str.length; i<strLen; i++) {
    bufView[i] = str.charCodeAt(i);
  }
  bufView[str.length] = 0;
  return buf;
}

var kernel32 = loadLibrary("kernel32.dll");
var GetModuleHandle = kernel32.getProc("GetModuleHandleA", "i", "stdcall");
var GetLastError = kernel32.getProc("GetLastError", "", "stdcall");

var user32 = loadLibrary("user32.dll");
var MessageBox = user32.getProc("MessageBoxA", "icci", "stdcall");
var RegisterClassEx = user32.getProc("RegisterClassExA", "s", "stdcall");
var CreateWindowEx = user32.getProc("CreateWindowExA", "icciiiiiiiii", "stdcall");
var DefWindowProc = user32.getProc("DefWindowProcA", "iiii", "stdcall");
var ShowWindow = user32.getProc("ShowWindow", "ii", "stdcall");
var UpdateWindow = user32.getProc("UpdateWindow", "i", "stdcall");
var GetMessage = user32.getProc("GetMessageA", "siii", "stdcall");
var TranslateMessage = user32.getProc("TranslateMessage", "s", "stdcall");
var DispatchMessage = user32.getProc("DispatchMessageA", "s", "stdcall");
var DestroyWindow = user32.getProc("DestroyWindow", "i", "stdcall");
var PostQuitMessage = user32.getProc("PostQuitMessage", "i", "stdcall");
var SendMessage = user32.getProc("SendMessageA", "iiii", "stdcall");
var BeginPaint = user32.getProc("BeginPaint", "is", "stdcall");
var EndPaint = user32.getProc("EndPaint", "is", "stdcall");

var gdi32 = loadLibrary("gdi32.dll");
var GetStockObject = gdi32.getProc("GetStockObject", "i", "stdcall");
var TextOut = gdi32.getProc("TextOutA", "iiici", "stdcall");

var gdiplus = loadLibrary("GDIPlus.dll");
var gdiplusStartup = gdiplus.getProc("GdiplusStartup", "ssi", "stdcall");
var gdiplusShutdown = gdiplus.getProc("GdiplusShutdown", "i", "stdcall");
var gdipLoadImageFromFile = gdiplus.getProc("GdipLoadImageFromFile", "ws", "stdcall");
var gdipDisposeImage = gdiplus.getProc("GdipDisposeImage", "i", "stdcall");
var gdipCreateFromHDC = gdiplus.getProc("GdipCreateFromHDC", "is", "stdcall");
var gdipDeleteGraphics = gdiplus.getProc("GdipDeleteGraphics", "i", "stdcall");
var gdipDrawImageI = gdiplus.getProc("GdipDrawImageI", "iiii", "stdcall");

var gdiplusStatusMessages = [
    "OK",
    "GenericError",
    "InvalidParameter",
    "OutOfMemory",
    "ObjectBusy",
    "InsufficientBuffer",
    "NotImplemented",
    "Win32Error",
    "WrongState",
    "Aborted",
    "FileNotFound",
    "ValueOverflow",
    "AccessDenied",
    "UnknownImageFormat",
    "FontFamilyNotFound",
    "FontStyleNotFound",
    "NotTrueTypeFont",
    "UnsupportedGdiplusVersion",
    "GdiplusNotInitialized",
    "PropertyNotFound",
    "PropertyNotFound",
    "ProfileNotFound"
];

var MB_OK = 0x00000000;
var MB_ICONEXCLAMATION = 0x00000030;

var WS_EX_CLIENTEDGE = 0x00000200;

var WS_OVERLAPPED   = 0x00000000;
var WS_POPUP        = 0x80000000;
var WS_CHILD        = 0x40000000;
var WS_MINIMIZE     = 0x20000000;
var WS_VISIBLE      = 0x10000000;
var WS_DISABLED     = 0x08000000;
var WS_CLIPSIBLINGS = 0x04000000;
var WS_CLIPCHILDREN = 0x02000000;
var WS_MAXIMIZE     = 0x01000000;
var WS_CAPTION      = 0x00C00000;
var WS_BORDER       = 0x00800000;
var WS_DLGFRAME     = 0x00400000;
var WS_VSCROLL      = 0x00200000;
var WS_HSCROLL      = 0x00100000;
var WS_SYSMENU      = 0x00080000;
var WS_THICKFRAME   = 0x00040000;
var WS_GROUP        = 0x00020000;
var WS_TABSTOP      = 0x00010000;
var WS_MINIMIZEBOX  = 0x00020000;
var WS_MAXIMIZEBOX  = 0x00010000;
var WS_OVERLAPPEDWINDOW = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

var SW_SHOWDEFAULT = 10;

var WM_CREATE  = 0x0001;
var WM_DESTROY = 0x0002;
var WM_PAINT   = 0x000F;
var WM_CLOSE   = 0x0010;
var WM_SETFONT = 0x0030;
var WM_COMMAND = 0x0111;

var COLOR_WINDOWFRAME = 6;

var hInstance = GetModuleHandle(0);

var startupInput = new DataView(new ArrayBuffer(16));
startupInput.setUint32(0, 1,  true);   // GdiplusVersion
startupInput.setUint32(4, 0,  true);   // DebugEventCallback
startupInput.setUint32(8, 0,  true);   // SuppressBackgroundThread
startupInput.setUint32(12, 0, true);   // SuppressExternalCodecs

var gdiplusStatus = 0;
var gdiplusToken = new DataView(new ArrayBuffer(4));
gdiplusStatus = gdiplusStartup(gdiplusToken.buffer, startupInput.buffer, 0);
if(gdiplusStatus != 0) {
    MessageBox(0, "gdiplusStartup failed - " + gdiplusStatusMessages[gdiplusStatus], "Error", MB_OK | MB_ICONEXCLAMATION);
    exit(-1);
}

var image = new DataView(new ArrayBuffer(4));
gdiplusStatus = gdipLoadImageFromFile("examples/lena.jpg", image.buffer);
if(gdiplusStatus != 0) {
    MessageBox(0, "GdipLoadImageFromFile failed - " + gdiplusStatusMessages[gdiplusStatus], "Error", MB_OK | MB_ICONEXCLAMATION);
    exit(-1);
}

var onPaint = function(hwnd, msg, wParam, lParam) {
    var paintStruct = new DataView(new ArrayBuffer(64));
    var hdc = BeginPaint(hwnd, paintStruct.buffer);
    TextOut(hdc, 90, 0, "lena.png", 8);

    var graphics = new DataView(new ArrayBuffer(4));
    var gdiplusStatus = gdipCreateFromHDC(hdc, graphics.buffer);
    if(gdiplusStatus != 0) {
        MessageBox(0, "gdipCreateFromHDC failed - " + gdiplusStatusMessages[gdiplusStatus], "Error", MB_OK | MB_ICONEXCLAMATION);
        exit(-1);
    }

    gdiplusStatus = gdipDrawImageI(graphics.getUint32(0, true), image.getUint32(0, true), 0, 20);
    if(gdiplusStatus != 0) {
        MessageBox(0, "gdipDrawImageI failed - " + gdiplusStatusMessages[gdiplusStatus], "Error", MB_OK | MB_ICONEXCLAMATION);
        exit(-1);
    }

    gdiplusStatus = gdipDeleteGraphics(graphics.getUint32(0, true));
    if(gdiplusStatus != 0) {
        MessageBox(0, "gdipDeleteGraphics failed - " + gdiplusStatusMessages[gdiplusStatus], "Error", MB_OK | MB_ICONEXCLAMATION);
        exit(-1);
    }

    EndPaint(hwnd, paintStruct.buffer);
}

var LOWORD = function(l) {
    return l & 0xffff;
}
var HIWORD = function(l) {
    return (l >> 16) & 0xffff;
}

var wndProc = new CallbackFunction("iiii", "stdcall", function(hwnd, msg, wParam, lParam) {
    switch(msg) {
    case WM_PAINT:
        onPaint(hwnd, msg, wParam, lParam);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
});

var className = str2ab("TEST");

var wndClass = new DataView(new ArrayBuffer(48));
wndClass.setUint32(0, 48,                         true);    // cbSize
wndClass.setUint32(4, 0,                          true);    // style
wndClass.setUint32(8, wndProc.getAddress(),       true);    // lpfnWndProc
wndClass.setUint32(12, 0,                         true);    // cbClsExtra
wndClass.setUint32(16, 0,                         true);    // cbWndExtra
wndClass.setUint32(20, hInstance,                 true);    // hInstance
wndClass.setUint32(24, 0,                         true);    // hIcon
wndClass.setUint32(28, 0,                         true);    // hCursor
wndClass.setUint32(32, COLOR_WINDOWFRAME,         true);    // hbrBackground
wndClass.setUint32(36, 0,                         true);    // lpszMenuName
wndClass.setUint32(40, getBaseAddress(className), true);    // lpszClassName
wndClass.setUint32(44, 0,                         true);    // hIconSm

if(RegisterClassEx(wndClass.buffer) == 0) {
    MessageBox(0, "RegisterClassEx failed", "Error", MB_OK | MB_ICONEXCLAMATION);
    exit(-1);
}

var hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "TEST", "My Window", WS_OVERLAPPEDWINDOW, 10, 10, 350, 350, 0, 0, hInstance, 0);
if(hwnd == 0) {
    MessageBox(0, "CreateWindowEx failed", "Error", MB_OK | MB_ICONEXCLAMATION);
    exit(-1);
}

ShowWindow(hwnd, SW_SHOWDEFAULT);
UpdateWindow(hwnd);

var msg = new DataView(new ArrayBuffer(28));

while(GetMessage(msg.buffer, 0, 0, 0) > 0) {
    TranslateMessage(msg.buffer);
    DispatchMessage(msg.buffer);
}


gdiplusStatus = gdipDisposeImage(image.getUint32(0, true));
if(gdiplusStatus != 0) {
    MessageBox(0, "GdipDisposeImage failed - " + gdiplusStatusMessages[gdiplusStatus], "Error", MB_OK | MB_ICONEXCLAMATION);
    exit(-1);
}

gdiplusShutdown(gdiplusToken.getUint32(0, true));
