from ubuntu

env CLANGVERSION=17
env WASMTIMEAPIVERSION=v18.0.3
env PLATFORM=x86_64

# install deps
run apt update
run apt install -y build-essential xz-utils git wget coreutils \
      lsb-release software-properties-common gnupg libglfw3 libglfw3-dev \
      libglm-dev g++-10 curl

env HOME="/root"

# get clang
run wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh \ 
      && ./llvm.sh ${CLANGVERSION}
env CC=clang-${CLANGVERSION} CXX=clang++-${CLANGVERSION} 

# get wasmtime c api
env WASMTIMEAPI=$HOME/wasmtime-api
run wget https://github.com/bytecodealliance/wasmtime/releases/download/${WASMTIMEAPIVERSION}/wasmtime-${WASMTIMEAPIVERSION}-${PLATFORM}-linux-c-api.tar.xz
run tar -xvf wasmtime-${WASMTIMEAPIVERSION}-${PLATFORM}-linux-c-api.tar.xz
run mv wasmtime-${WASMTIMEAPIVERSION}-${PLATFORM}-linux-c-api $WASMTIMEAPI

# get and build glfw source to get static lib on linux x86_64
run apt install -y xorg-dev libwayland-dev libxkbcommon-dev wayland-protocols cmake extra-cmake-modules
workdir $HOME
run git clone https://github.com/glfw/glfw.git  # might want to pin this with && cd glfw && git checkout 3.3.2
run cmake -S /root/glfw -B /root/glfw/build -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF
run cd /root/glfw/build && make
env GLFWLIB=/root/glfw/build/src
