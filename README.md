This is my first ever project with wasm, now modded into AI project on bayesian network.

This is a client side only webproject, but since one needs to serve wasm files to run,
you need to have a simple http server mechanism for running this app.


Compilation of app:
-- To compile the web app only, a batch script is provided, named 'compile-wasm.bat'. As long as you have clang on your system and it is on path, it can compile wasm, even on android in termux.

-- A simple HTTP server app written in C is also available for windows platform, which can be compiled along with wasm app by 'compile.bat' batch script


Using the app:

-- This app can be used as long as there is any simple HTTP server app in the system and you know how to run it.
-- There are two methods tested, which are :
-- Method 1: Using python, if you have python 3 or greater, then simply open any command window in the folder of this app, and run
    python -m http.server 12706

-- Method 2: If you are on windows, and have compiled the simple_server.c file, you can use the 'run.bat' batch script to run the http server. You can also use the simple_server.exe executable directly, it just requires port number as first argument, then a pair of HTTP response mime types and corresponding files to serve. In the 'run.bat', it is setup so that webapp serves also at the port address 12706.

-- After either of these processes, the webapp will be served on port 12706, so you can access the webapp by simply going to browser to 'localhost:12706'.

If you want to 'browse' the app source codes:

index.html :
-- This is the only html file, that declares some div elements, a canvas element and loads the relevant js files.

wasm.js :
-- This is the wasm interface helper js file that only sets up loading and providing some functions that help with interoperability with the C struct used in webapp.

divs.js :
-- This is a collection of some js functions used in making the visualizations.

bayes.js :
-- This is the main visualization file. In this file is where the visualization logic relevant to the bayesian network is written. This file communicates with the wasm module using some helper functions in wasm.js and is reponsible for maintaing the correct correspondance between C and js.

bitmap_alloc.c , t1.c and t1.h :
-- Since this is also my first wasm project, and I have not used any external libraries, not even the standard libraries in C, these 3 files help to setup some of the necessary components, like dynamic memory allocation in C, some memory copying functions, and some functions to log stuff to the js console. Not required at all if you are interested only in bayesian network.

bayes.c :
-- This file is currently the entirety of the bayesian network logic. This code is reponsible for handling the creation of nodes and creation of directed edge between the nodes when instructed by the js component. Currently no bayesian network inference logic is built.


simple_server.c :
-- As mentioned earlier, this if a file that sets up a simple http server for windows platform only. This uses windows socket programming to setup the server. This code is irrevalant if you are using linux or python's server. 