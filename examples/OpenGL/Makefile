
ifdef CXX
	clang := $(CXX)
else
	clang := clang++
endif

ifdef WASMTIMEAPI
  wasmtimeapi := $(WASMTIMEAPI)
else
  wasmtimeapi := ../../wasmtime
endif
libwasmtime := $(WASMTIMEAPI)/lib/libwasmtime.a

ifdef GLFWLIB
  libglfw := $(GLFWLIB)/libglfw3.a
else
	libglfw := glfw/lib-arm64/libglfw3.a
endif


options := -std=c++17  -fms-extensions \
           -Wno-error=extra-tokens -Wno-quoted-include-in-framework-header -O3
includes := -I$(wasmtimeapi)/include -I../../
link :=  $(libglfw) $(libwasmtime) 

# extras for non-linux platform build
optionsextra := -stdlib=libc++
includesextra := -I -Iglm -Iglfw/include
linkextra := -framework Foundation -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo

sources = demo.cpp ../../wander.cpp
objects = $(sources:.cpp=.o)
target = opengl

# clang wants this on lnx
CXXFLAGS = $(includes) $(options)  

buildlinux: $(objects)
	$(clang) $(objects) `pkg-config --libs glx` $(link) -o $(target) 

build:
	$(clang) $(includes) $(includesextra) \
		$(options) $(optionsextra) \
		$(link) $(linkextra) -o $(target) $(sources)

clean: 
	rm -f $(target) $(objects)

run: build
	./opengl
