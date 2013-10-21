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

var gdi32 = loadLibrary("gdi32.dll");
var GetStockObject = gdi32.getProc("GetStockObject", "i", "stdcall");

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

var BS_DEFPUSHBUTTON = 0x00000001;

var SW_SHOWDEFAULT = 10;

var WM_CREATE  = 0x0001;
var WM_DESTROY = 0x0002;
var WM_CLOSE   = 0x0010;
var WM_SETFONT = 0x0030;
var WM_COMMAND = 0x0111;

var COLOR_WINDOWFRAME = 6;

var DEFAULT_GUI_FONT = 17;

var BTN_HELLO_WORLD = 100;

var hInstance = GetModuleHandle(0);

var onCreate = function(hwnd, msg, wParam, lParam) {
    var defaultFont = GetStockObject(DEFAULT_GUI_FONT);
    var button = CreateWindowEx(0, "BUTTON", "Hello World!", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 160, 40, 80, 24, hwnd, BTN_HELLO_WORLD, hInstance, 0);
    SendMessage(button, WM_SETFONT, defaultFont, 0);
}

var LOWORD = function(l) {
    return l & 0xffff;
}
var HIWORD = function(l) {
    return (l >> 16) & 0xffff;
}

var wndProc = new CallbackFunction("iiii", "stdcall", function(hwnd, msg, wParam, lParam) {
    switch(msg) {
    case WM_CREATE:
        onCreate(hwnd, msg, wParam, lParam);
        return 0;
    case WM_COMMAND:
        if(LOWORD(wParam) == BTN_HELLO_WORLD) {
            MessageBox(hwnd, "Hello World!", "Info", MB_OK);
            return 0;
        }
        break;
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

var hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "TEST", "My Window", WS_OVERLAPPEDWINDOW, 10, 10, 400, 300, 0, 0, hInstance, 0);
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
