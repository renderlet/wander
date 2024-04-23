# wander - the Wasm Renderer

![logo light](/docs/wander-light.svg#gh-light-mode-only)
![logo dark](/docs/wander-dark.svg#gh-dark-mode-only)

`wander` is a GPU-based rendering framework designed to be fully embeddable in any application.

Use `wander` to run `renderlets` in any software - self contained, portable modules of graphics data and code compiled to WebAssembly. 


## 🗣️ Join Us On Discord

Let's discuss the intersection of Wasm and Graphics!

[![Discord Banner 3](https://discordapp.com/api/guilds/1232052216857231443/widget.png?style=banner2)](https://discord.gg/NzmehuETUu)

## What can this do?

With renderlet, developers can build modern, interactive visualizations that run on any platform.

Many visual apps need a high-performance rendering engine that works in a browser or in a native app, where a JavaScript library won't scale, but a game engine is not the right application model. 

`wander` is designed to be a rendering engine for any high-performance application. It primarily is designed as the runtime to run `renderlet` bundles, but also can be used more generically as a WebAssembly-based rendering engine that uses the GPU.

### What are renderlets?

The `wander` SDK brings all of the benefits of `renderlets` into your own application:
* High performance 2D and 3D graphics
* Fully portable rendering pipelines
    * Any app, device, client, or server
    * Sandboxed third party code execution
* Self contained rendering modules
    *  Reuse your graphics code as building blocks
* No game engine / platform dependencies
* No GPU expertise required

### Example use-cases

Use `wander` to build things like:
* Modern CAD Apps: Use code to allow objects to describe how to render themselves
* Procedural Modeling and DataViz: Execute runtime code to view your data in new ways
* Product Visualization: Interact with dynamic objects in 3D on any device
* Composable User Interfaces: Render any UI module on top of an existing canvas or in any scene
* Interactive Animations: Like Adobe Flash, but with an actual programming language and GPUs
* Low/No-Code Graphics: Make a graphics pipelines out of existing building blocks

## Platform Support

| API    | Windows            | Linux/Android      | macOS/iOS          | Web (wasm)         |
| ------ | ------------------ | ------------------ | ------------------ | ------------------ |
| OpenGL | :white_check_mark: (GL 3.3)      | :hammer_and_wrench: (GL ES 3.0+)  | :white_check_mark: | :triangular_ruler: (WebGL2)      |
| DX11   | :white_check_mark: |                    |                    |                    |
| DX12   | :hammer_and_wrench: |                    |                    |                    |
| WebGPU |                    |                    |                    | :hammer_and_wrench: |
| Vulkan | :triangular_ruler: | :triangular_ruler: | :triangular_ruler: |                    |
| Metal  |                    |                    | :triangular_ruler: |                    |

:white_check_mark: = Currently supported
:hammer_and_wrench: = In progress, coming soon
:triangular_ruler: = In design or planned for the future


## Quickstart

`wander` is delivered as a cross-platform C++ library. Simply add wander.h/wander.cpp to your build chain along with the included dependencies such as `wasmtime`.

The basic API flow is:
```C++
const auto pal = wander::Factory::CreatePal(
        wander::EPalType::D3D11, baseDevice, baseDeviceContext);
auto runtime = wander::Factory::CreateRuntime(pal);

auto renderlet_id = runtime->LoadFromFile(L"Building.wasm", "run");

auto tree_id = runtime->Render(renderlet_id);
auto tree = runtime->GetRenderTree(tree_id);
```
1. Create a PAL object based on your device's underlying GPU API
2. Create a global `runtime` object. This is not currently thread safe
3. Load a `renderlet` file (with an optional entry point name) and get it ready to run
4. Run it! For static content this can be done once, but dynamic content could be per frame
5. Get a (non-owning) pointer to the output

Drawing the data is as simple as iterating the render tree:
```C++
deviceContext->PSSetShaderResources(0, 1, &textureViewWhite);

for (auto i = 0; i < tree->Length(); ++i)
{
    tree->NodeAt(i)->RenderFixedStride(runtime, stride);
}
```
There's no need to manage the underlying graphics data / buffers - `wander` takes care of it.

To delete/free old content, simple call:
```C++
runtime->DestroyRenderTree(tree_id);
```

Also, wander can tear down every object automatically on unload or close:
```C++
runtime->Release();
```

### Usage

Examples are provided in the [examples folder](examples/) for D3D11 and OpenGL 3.3 (Mac/Windows/Linux).

CMake support is not yet implmented, but you can follow platform-specific examples below to integrate into your own app.

#### rive-renderer integration

`wander` has experimental support for 2D vector operations using [`rive-renderer`](https://github.com/rive-app/rive-renderer)

Currently this is only supported in the Windows / Direct3D11 backend. We are working on a `WebGPU` build, and MacOS will require us to implement a `Metal` backend.

It is non-trivial to build `rive-renderer` on Windows. We have provided a GitHub release with precompiled binaries / .lib files in [the D3D11 example](examples/DX11). If you want to build yourself, here's some tips:
1. Ensure you have a `MinGW` toolchain installed
2. Clone this repo with `--recurvsive` - `rive-renderer` will be subbomduled in `libs`, and will have `rive-cpp` submoduled in `libs/rive-renderer/submodules/rive-cpp`
3. Follow the basic instructions using `MinGW` for commands in the `README` for `rive-renderer`
4. You may manually have to run CMake for glfw: `cmake ../glfw -DBUILD_SHARED_LIBS=OFF -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -DGLFW_BUILD_WAYLAND=OFF -A x64`
5. For premake, `set PREMAKE_PATH=<path>wander\libs\rive-renderer\submodules\rive-cpp\build`
6. Then, `premake5 vs2022 --toolset=msc --with_rive_text --config=release --out=out/release` (or debug)
7. Open the output `rive.sln`, and switch every project to `MD` / `MDd` to match most apps
8. For `path_fiddle`, upgrade to C++20 and on linker and C++ settings disable warnings as errors (`/WX-`)
9. Now build all in Visual Studio or via MSBuild - the DX11 example will automatically pull from the submodule paths

To use this, `RLT_RIVE` globally in your project (the DX11 example does this automatically in the Release build), and `wander` will automatically link to the output `.lib`s.

You can read more about the `rive-renderer` [integration here](https://github.com/renderlet/wander/wiki/Using-renderlet-with-rive%E2%80%90renderer).

![animated](https://github.com/renderlet/wander/blob/main/docs/wiki/vector-static.gif)

#### Building the Examples

##### On Windows
Ensure you have the latest Visual Studio 2022 and Windows 11 SDK, and use the `Examples.sln`.

The DX11 example includes experimental `rive-renderer` support for 2D vectors.

##### On OSX

Ensure you have a working `clang` installation, then simply use the [`Makefile`](examples/OpenGL/Makefile):
```sh
make run
```

This should produce a new `opengl` binary under `/src/examples/OpenGL` and run it with the default scene.

##### On Linux

First, build the build container. From the repository root:

```sh
podman build -t wander/buildimage -f ./Dockerfile
```

Then, use this container to build or rebuild the examples. To build the OpenGL example on Linux:

```sh
podman run --mount=type=bind,source="$(pwd)",target=/src,relabel=shared,U=true -w /src/examples/OpenGL wander/buildimage make buildlinux
```

This should produce a new `opengl` binary under `/src/examples/OpenGL`. Take a look at the `Makefile` for more info.

### :warning: Building renderlets

An example `renderlet` is provided in the examples - a 3D procedural model of a building - `building.wasm`.

We are currently working on a compiler and tool to make it easy to build your own renderlets.

If you want to experiment with building your own through raw wasm code, the wire format and signatures [can be found here](https://github.com/renderlet/wander/blob/e86d549606e24a04ae4b25544336b2744aec4ce0/wander.cpp#L331).

This format can and will change over time, so please only experiment with this if you are ok with breaking your renderlet on upgrade!

## Features

| Feature    | Supported            | Coming Soon    | Future        |
| ------ | ------------------ | ------------------ | ------------------ | 
| Untyped buffers | :white_check_mark:    |  | 
| Vertex buffers   | :white_check_mark:   |                    |                    | 
| Geometry/Mesh support   | :white_check_mark:   |                    |                    |
| Materials |  :white_check_mark: (*) |                    |                    |
| Index buffers | | :hammer_and_wrench: |  |
| Texture data  |   | :hammer_and_wrench:  |  |
| CMake / x-platform build |   | :hammer_and_wrench:  |  |
| C API / other langs  |   | :hammer_and_wrench:  |  |
| Buffer format specs  |   |   | :triangular_ruler: |
| Dynamic render trees  |   |   | :triangular_ruler: |
| Conditional rendering  |   |   | :triangular_ruler: |
| Shaders  |   |   | :triangular_ruler: |
| Distributed GPU compute  |   |   | :triangular_ruler: |
| Web build  |   | :hammer_and_wrench:  |  |
| Renderlet compiler toolset <br>(easily build your own renderlets) |   | :hammer_and_wrench: (*) |  |

*unstable API

Feature requests welcome, please submit an issue!

## Architecture / Roadmap 

Current (V1) architecture allows arbitray data to be generated on the CPU in Wasm which is automatically uploaded to the GPU via the `wander` SDK through the PAL.

The host app must know the "shape" of the data and bind a shader stage to the host graphics API to render the geometry / texture data to the Canvas.

Example - CAD, where arbitrary parts can generate geometry of a known format that the host app has the capability to shade

![v1 light](/docs/v1-light.png#gh-light-mode-only)
![v1 dark](/docs/v1-dark.png#gh-dark-mode-only)

Future (V2) architecture allows arbitrary data to be fully rendered to an application's back buffer / canvas via an attachment to the graphics API exposed to Wasm.

V2 will enable fully arbitrary rendering of any data onto any Canvas. V2 has a dependency on the upcoming `wasi-gpu` spec.

Example - Full 3rd-party UGC/Content - Rendering arbitrary 2D gauges (like Flash) / 3D content (avatars) onto an existing Canvas layer in an app

![v2 light](/docs/v2-light.png#gh-light-mode-only)
![v2 dark](/docs/v2-dark.png#gh-dark-mode-only)
