var testdll = loadLibrary("testdll.dll");
var a = 42;
var b = 43;

print("CDECL");

var Sum = testdll.getProc("Sum", "iii", "cdecl");
var result = Sum(a, b, new CallbackFunction("ii", "cdecl", function(x, y) {
    print(x + " + " + y);
}).getAddress());

print("= " + result);
print();

print("STDCALL");

var Sum2 = testdll.getProc("_Sum2@12", "iii", "stdcall");
var result2 = Sum2(a, b, new CallbackFunction("ii", "stdcall", function(x, y) {
    print(x + " + " + y)
}).getAddress());

print("= " + result2);
print();

print("STRUCT");

var intStruct = new DataView(new ArrayBuffer(4));
intStruct.setUint32(0, 42, true);
var Struct = testdll.getProc("Struct", "s", "cdecl");
var result = Struct(intStruct.buffer);
print("Returned: " + result);

print();

print("STRUCT (NULL)");

var Struct = testdll.getProc("Struct", "s", "cdecl");
var result = Struct(null);
print("Returned: " + result);
