Rem clang --target=wasm32 -nostdlib -Wl,--no-entry,--export-all,--unresolved-symbols=import-dynamic -o t1.wasm t1.c bitmap_alloc.c -DSIZE_TYPE="unsigned int" -O2 -Wall -Wno-incompatible-library-redeclaration %*
clang --target=wasm32 -nostdlib -Wl,--no-entry,--export-all,--unresolved-symbols=import-dynamic -o bayes.wasm bayes.c t1.c bitmap_alloc.c -DSIZE_TYPE="unsigned int" -O2 -Wall -Wno-incompatible-library-redeclaration %* 	
clang simple_serv.c -o simple_serv.exe -O2 -lws2_32 -Wall  %* 
