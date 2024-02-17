This is my first ever project with wasm.
This project contains two parts, a simple http server written in C using Windows websockets,
another is a client web application written in C wasm, html and js

If you don't have windows, you can still use other tools that function similar to this toy webserver {like in python }

Purposes of files : 


bitmap_alloc.c :

   This is my first ever pathetic attempt at a general purpose allocator, not thread safe at all. 
   This file is independent and only requires a few extern functions to work properly, but it assumes the underlying memory region is contiguous, which is quite fitting for wasm and also works for other linux systems with brk() sbrk() family.


bitmap2.c :

    This file is a test implementation of bitmap_alloc.c in plain C.
    Compilation instructions are not explicitly provided for this file though.
    Maybe later will be provided, or this file will be entirely removed.


simple_serv.c :

    This file is the simple HTTP server written to serve the wasm file to the browser, along with html file, as browsers cannot simply load such binaries. It is written with Win32 API in mind, but if needed translating to posix sockets shouldnot be that hard.


t1.c :

    This is the main wasm file that the browser loads.
    There is no emscripten used, no standatd library used, as this was just a learning attempt, and as such is free of such bloat for now.
    The compilation was done by clang in windows, but should work anywhere if proper flags are provided.
    This assumes wasm32, which is current default wasm, so don't compile as wasm64.


t1.html :

    This is the host html file for wasm.


t1.js :

    This is the js driver code for wasm. It actually sets up wasm interface, sets the timers and calls the C functions in said time intervals


compile.bat :

    This batch file compiles the simple_serv.c, as normal C executable (simple_serv.exe) and t1.c as a wasm file (t1.wasm) later loaded by t1.html.
    If in windows and clang is in path, and hoping that clang comes with wasm32, {which is defualt I think} , just run this batch file to compile both


run.bat :

    This file starts up the http server at port 12706.
    Just goto localhost:12706 after this runs.
    This simply provies t1.html and t1.wasm as arguments to simple_serv.exe.
    