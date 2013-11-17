# jswin - Loading dynamic link libraries in JavaScript

jswin is a simple runtime environment based on Google V8 that allows calling functions in DLLs from pure JavaScript. The functions must follow the stdcall or cdecl calling convention. Arguments must be supplied as C compatible data types, such as integers or structs (JavaScript Typed Arrays). Strings are automatically converted to UTF-8. JavaScript functions can be converted to function pointers callable from C code in the library.

## Examples

Loading a DLL and calling a function with multiple arguments:

```javascript
var user32 = loadLibrary("user32.dll");
var MessageBox = user32.getProc("MessageBoxA", "icci", "stdcall");
MessageBox(0, "Hello World!", "Info", 0);
```

Using callback functions:

```javascript
var onMouseDownHandler = new CallbackFunction("ii", "stdcall", function(x, y) {
    // ...
});

var SetOnMouseDownHandler = ....
SetOnMouseDownHandler(onMouseDownHandler.getAddress());
```

## API Reference

`loadLibrary(dllName)` returns `DLLObject`

Loads the library with the given name.

`getBaseAddress(array)` returns `number`

Returns the base address of an `ArrayBuffer`. This is the memory address of the first element in the array. The address remains valid as long as the `ArrayBuffer` is not deleted by the garbage collector.

`exit(status)`

Terminates the process. Set `status` to `0` to indicate success. Use another status code to indicate failure.

`DLLObject.getProc(procName, signature, callingConvention) returns `DLLProc`

Retrieves the function named `procName` from the DLL. When calling the function from JavaScript, the arguments are converted as specified in `signature`. `callingConvention` must be `"stdcall"` or `"cdecl"`.

`signature` is a string in which each character specifies the C compatible type of an argument. The following types are supported:

* `i` integer
* `c` ANSI string
* `w` wide character string
* `s` struct or array

JavaScript strings are converted to UTF-8 encoded, null-terminated byte arrays (C strings). Structs or arrays have to be supplied as `ArrayBuffer`.

`DLLProc(...)` returns `number`

Calls the function of the corresponding DLL. The result of the function is returned as an integer.

`new DLLProc(signature, callingConvention, address)` returns `number`

Constructs a new object describing a function callable from C. `signature` and `callingConvention` are identical to `DLLObject.getProc(...)`. `address` is the memory address of the function. Usually, instances of `DLLProc` are constructed by calling `DLLObject.getProc(...)` instead of using this constructor function.

`DLLProc.getAddress()` returns `number`

Returns the address of the function corresponding to the `DLLProc` object.

`new CallbackFunction(signature, callingConvention, func)` returns `CallbackFunction`

Constructs a new object describing a JavaScript function callable from C. `signature` and `callingConvention` are identical to `DLLObject.getProc(...)`. `func` is a JavaScript function.

`CallbackFunction.getAddress()` returns `number`

Returns a C compatible address for the JavaScript function corresponding to the `CallbackFunction` object. This address can be passed as argument when calling a function from the DLL. The address remains valid as long as the `CallbackFunction` is not deleted by the garbage collector.

`require(moduleId)` returns `Object`

Loads and executes code in another JavaScript file. Based on [CommonJS Modules 1.1.1](http://wiki.commonjs.org/wiki/Modules/1.1.1).

## Compiling

This source code package contains project files for Microsoft Visual C++ 2012.

Dependencies:

* Google V8, Version 3.22.15 or compatible

## License

MIT License
