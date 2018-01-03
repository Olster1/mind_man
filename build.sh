
gcc -w meta_generator.cpp -o meta_program.out -std=gnu++11 -DMAC_PORT=1 
./meta_program.out
gcc -o mind_man.out calm_sdl.cpp -w -lSDL2 -std=gnu++11 -framework OpenGL -g

#-L/usr/local/lib -I/usr/local/include/SDL2 -D_THREAD_SAFE -framework Cocoa 
# -I Add the specified directory to the search path for include files. 
# -D Defines a macro name before the preprocessor is defined. 