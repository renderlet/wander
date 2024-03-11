clang := clang++
includes := -I -Iglm -Iglfw/include -I../../wasmtime/include -I../../
options := -std=c++17 -stdlib=libc++ -fms-extensions \
           -Wno-error=extra-tokens -Wno-quoted-include-in-framework-header -O3
link := glfw/lib-arm64/libglfw3.a ../../wasmtime/lib/libwasmtime.a \
         -framework Foundation -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo

build:
	$(clang) $(includes) $(options) $(link) \
	   -o opengl demo.cpp ../../wander.cpp

run: build
	./opengl