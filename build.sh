@echo off
gcc -w meta_generator.cpp -o meta_program.out -std=gnu++11 -DMAC_PORT=1 -O0
./meta_program.out
pushd ../../art_assets
gcc -o mind_man ../Apps/mind_man-master/calm_sdl.cpp -w -lSDL2 -std=gnu++11 -framework OpenGL -O0 -g -DMAC_PORT -DINTERNAL_BUILD
popd
#-g for debug symbols file
#-L/usr/local/lib -I/usr/local/include/SDL2 -D_THREAD_SAFE -framework Cocoa 
# -I Add the specified directory to the search path for include files. 
# -D Defines a macro name before the preprocessor is defined. 