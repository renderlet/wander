# wander
wander - the Wasm Renderer

`wander` is a runtime library for composable rendering in WebAssembly.

Embed `wander` into any application, and host graphical components called `renderlets` that are self contained modules of graphics data and code. Each `renderlet` is a Wasm module that can safely reuse graphics code to render any data into any app on any device.

## Architecture

Current (V1) architecture allows arbitray data to be generated on the CPU in Wasm which is automatically uploaded to the GPU via the `wander` SDK through the PAL.

The host app must know the "shape" of the data and bind a shader stage to the host graphics API to render the data.

![image info](/docs/v1.png)

Future (V2) architecture allows arbitrary data to be fully rendered to an application's back buffer / canvas via an attachment to the graphics API exposed to Wasm.

V2 will enable fully arbitrary rendering of any data onto any Canvas. V2 has a dependency on the upcoming `wasi-gpu` spec.

![image info](/docs/v2.png)