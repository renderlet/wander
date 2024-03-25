#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS

#pragma comment(lib, "user32")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#pragma comment(lib, "wasmtime.dll.lib")

///////////////////////////////////////////////////////////////////////////////////////////////////

#define NOMINMAX

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include <fstream>
#include <math.h> // sin, cos
#include "xube.h" // 3d model

#include "bmpread.h"

#include "wander.h"

#ifdef _DEBUG

#pragma comment(lib, "rive_pls_renderer.lib")
#pragma comment(lib, "rive.lib")
#pragma comment(lib, "rive_sheenbidi.lib")
#pragma comment(lib, "rive_harfbuzz.lib")
#pragma comment(lib, "rive_decoders.lib")
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "zlib.lib")

#include "rive/pls/pls_render_context.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/pls/d3d/pls_render_context_d3d_impl.hpp"
#include "rive/pls/d3d/d3d11.hpp"

using namespace rive;
using namespace rive::pls;

static float2 s_pts[] = {{260 + 2 * 100, 60 + 2 * 500}, {260 + 2 * 257, 60 + 2 * 233}, {260 + 2 * -100, 60 + 2 * 300},
						 {260 + 2 * 100, 60 + 2 * 200}, {260 + 2 * 250, 60 + 2 * 0},   {260 + 2 * 400, 60 + 2 * 200},
						 {260 + 2 * 213, 60 + 2 * 200}, {260 + 2 * 213, 60 + 2 * 300}, {260 + 2 * 391, 60 + 2 * 480}};

#endif


///////////////////////////////////////////////////////////////////////////////////////////////////

#define TITLE "renderlet - adapted from Minimal D3D11 by d7samurai"

///////////////////////////////////////////////////////////////////////////////////////////////////

struct float3 { float x, y, z; };
struct matrix { float m[4][4]; };

matrix operator*(const matrix& m1, const matrix& m2);

///////////////////////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WNDCLASSA wndClass = { 0, DefWindowProcA, 0, 0, 0, 0, 0, 0, 0, TITLE };

    RegisterClassA(&wndClass);

    HWND window = CreateWindowExA(0, TITLE, TITLE, WS_POPUP | WS_MAXIMIZE | WS_VISIBLE, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1 };
    
    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;

    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Device1* device;

    baseDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&device));

    ID3D11DeviceContext1* deviceContext;

    baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&deviceContext));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    IDXGIDevice1* dxgiDevice;

    device->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&dxgiDevice));

    IDXGIAdapter* dxgiAdapter;

    dxgiDevice->GetAdapter(&dxgiAdapter);

    IDXGIFactory2* dxgiFactory;

    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width              = 0; // use window width
    swapChainDesc.Height             = 0; // use window height
    //swapChainDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo             = FALSE;
    swapChainDesc.SampleDesc.Count   = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount        = 2;
    swapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD; // prefer DXGI_SWAP_EFFECT_FLIP_DISCARD, see Minimal D3D11 pt2 
    swapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags              = 0;

    IDXGISwapChain1* swapChain;

    dxgiFactory->CreateSwapChainForHwnd(device, window, &swapChainDesc, nullptr, nullptr, &swapChain);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Texture2D* frameBuffer;

    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer));

    ID3D11RenderTargetView* frameBufferView;

    device->CreateRenderTargetView(frameBuffer, nullptr, &frameBufferView);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_TEXTURE2D_DESC depthBufferDesc;

    frameBuffer->GetDesc(&depthBufferDesc); // copy from framebuffer properties

    depthBufferDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthBuffer;

    device->CreateTexture2D(&depthBufferDesc, nullptr, &depthBuffer);

    ID3D11DepthStencilView* depthBufferView;

    device->CreateDepthStencilView(depthBuffer, nullptr, &depthBufferView);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* vsBlob;

    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, nullptr);

    ID3D11VertexShader* vertexShader;

    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);

    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = // float3 position, float3 normal, float2 texcoord, float3 color
    {
        { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ID3D11InputLayout* inputLayout;

    device->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* psBlob;

    D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &psBlob, nullptr);

    ID3D11PixelShader* pixelShader;

    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_RASTERIZER_DESC1 rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = false;

    ID3D11RasterizerState1* rasterizerState;

    device->CreateRasterizerState1(&rasterizerDesc, &rasterizerState);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    ID3D11SamplerState* samplerState;

    device->CreateSamplerState(&samplerDesc, &samplerState);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable    = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc      = D3D11_COMPARISON_LESS;

    ID3D11DepthStencilState* depthStencilState;

    device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    struct Constants
    {
        matrix Transform;
        matrix Projection;
        float3 LightVector;
    };

    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth      = sizeof(Constants) + 0xf & 0xfffffff0; // round constant buffer size to 16 byte boundary
    constantBufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ID3D11Buffer* constantBuffer;

    device->CreateBuffer(&constantBufferDesc, nullptr, &constantBuffer);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.ByteWidth = sizeof(VertexData);
    vertexBufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData = { VertexData };

    ID3D11Buffer* vertexBuffer;

    device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.ByteWidth = sizeof(IndexData);
    indexBufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData = { IndexData };

    ID3D11Buffer* indexBuffer;

    device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width              = TEXTURE_WIDTH;  // in xube.h
    textureDesc.Height             = TEXTURE_HEIGHT; // in xube.h
    textureDesc.MipLevels          = 1;
    textureDesc.ArraySize          = 1;
    textureDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count   = 1;
    textureDesc.Usage              = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA textureData = {};
    textureData.pSysMem            = TextureData;
    textureData.SysMemPitch        = TEXTURE_WIDTH * sizeof(UINT); // 4 bytes per pixel

    ID3D11Texture2D* texture;

    device->CreateTexture2D(&textureDesc, &textureData, &texture);

    ID3D11ShaderResourceView* textureView;

    device->CreateShaderResourceView(texture, nullptr, &textureView);

    ///////////////////////////////////////////////////////////////////////////////////////////////

	textureDesc.Width = 1;
	textureDesc.Height = 1;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	textureData.pSysMem = TextureDataWhite;
	textureData.SysMemPitch = sizeof(UINT); // 4 bytes per pixel

	ID3D11Texture2D *textureWhite;

	device->CreateTexture2D(&textureDesc, &textureData, &textureWhite);

	ID3D11ShaderResourceView *textureViewWhite;

	device->CreateShaderResourceView(textureWhite, nullptr, &textureViewWhite);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    bmpread_t bitmap;
	bmpread("roof2.bmp", BMPREAD_TOP_DOWN | BMPREAD_ANY_SIZE  | BMPREAD_ALPHA, &bitmap);

	textureDesc.Width = bitmap.width;
	textureDesc.Height = bitmap.height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	textureData.pSysMem = bitmap.data;
	textureData.SysMemPitch = bitmap.width *4; // 4 bytes per pixel

	ID3D11Texture2D *textureRoof;

	device->CreateTexture2D(&textureDesc, &textureData, &textureRoof);

	ID3D11ShaderResourceView *textureViewRoof;

	device->CreateShaderResourceView(textureRoof, nullptr, &textureViewRoof);

	///////////////////////////////////////////////////////////////////////////////////////////////

	bmpread_t bitmap2;
	bmpread("window.bmp", BMPREAD_TOP_DOWN | BMPREAD_ANY_SIZE | BMPREAD_ALPHA, &bitmap2);

	textureDesc.Width = bitmap2.width;
	textureDesc.Height = bitmap2.height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	textureData.pSysMem = bitmap2.data;
	textureData.SysMemPitch = bitmap2.width * 4; // 4 bytes per pixel

	ID3D11Texture2D *textureWindow;

	device->CreateTexture2D(&textureDesc, &textureData, &textureWindow);

	ID3D11ShaderResourceView *textureViewWindow;

	device->CreateShaderResourceView(textureWindow, nullptr, &textureViewWindow);

	///////////////////////////////////////////////////////////////////////////////////////////////


    FLOAT backgroundColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };

    UINT stride = 11 * sizeof(float); // vertex size (11 floats: float3 position, float3 normal, float2 texcoord, float3 color)
    UINT offset = 0;

    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(depthBufferDesc.Width), static_cast<float>(depthBufferDesc.Height), 0.0f, 1.0f };
    
    ///////////////////////////////////////////////////////////////////////////////////////////////

    float w = viewport.Width / viewport.Height; // width (aspect ratio)
    float h = 1.0f;                             // height
    float n = 1.0f;                             // near
    float f = 9.0f;                             // far

    float3 modelRotation    = { 0.0f, 0.0f, 0.0f };
    float3 modelScale       = { 1.0f, 1.0f, 1.0f };
    float3 modelTranslation = { 2.0f, 0.0f, 4.0f };

    float3 wasmScale = {0.1f, 0.1f, 0.1f};
	float3 wasmTranslation = {-2.0f, 0.0f, 4.0f};

    ///////////////////////////////////////////////////////////////////////////////////////////////

    const auto pal = wander::Factory::CreatePal(
        wander::EPalType::D3D11, baseDevice, baseDeviceContext);
	auto runtime = wander::Factory::CreateRuntime(pal);

    std::wstring dwnld_URL = L"https://rltdemoapi2.azurewebsites.net/compiler/output";
	std::wstring savepath = L"demo.rlt";
	URLDownloadToFile(NULL, dwnld_URL.c_str(), savepath.c_str(), 0, NULL);

    auto renderlet_id = runtime->LoadFromFile(L"demo.rlt", "start");

	auto tree_id = runtime->Render(renderlet_id);
	auto tree = runtime->GetRenderTree(tree_id);

	///////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
    pls::PLSRenderContextD3DImpl::ContextOptions contextOptions{};

    std::unique_ptr<pls::PLSRenderContext> plsContext =
		pls::PLSRenderContextD3DImpl::MakeContext(baseDevice, baseDeviceContext, contextOptions);

    auto plsContextImpl = plsContext->static_impl_cast<PLSRenderContextD3DImpl>();

    D3D11_TEXTURE2D_DESC CoordinateTexDesc = {
		(UINT)bitmap.width, // UINT Width;
		(UINT)bitmap.height, // UINT Height;
		1, // UINT MipLevels;
		1, // UINT ArraySize;
		DXGI_FORMAT_R8G8B8A8_UNORM, // DXGI_FORMAT Format;
		{1, 0}, // DXGI_SAMPLE_DESC SampleDesc;
		D3D11_USAGE_DEFAULT, // D3D11_USAGE Usage;
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, // UINT BindFlags;
		0, // UINT CPUAccessFlags;
		0, // UINT MiscFlags;
	};


	ID3D11Texture2D*  ptex2DCoordinateTexture;
	ID3D11ShaderResourceView *ptex2DCoordinateTextureSRV;
	device->CreateTexture2D(&CoordinateTexDesc, NULL, &ptex2DCoordinateTexture);
	device->CreateShaderResourceView(ptex2DCoordinateTexture, NULL, &ptex2DCoordinateTextureSRV);

    auto renderTarget = 
        plsContextImpl->makeRenderTarget(
            CoordinateTexDesc.Width, CoordinateTexDesc.Height);
#endif
    ///////////////////////////////////////////////////////////////////////////////////////////////

    while (true)
    {
        MSG msg;

        auto quit = false;

        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_KEYDOWN)
            {
				quit = true;
				break;
            }
                
            DispatchMessageA(&msg);
        }

        if (quit)
        {
			break;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
		deviceContext->OMSetRenderTargets(1, &frameBufferView, nullptr);


		plsContext->beginFrame({
			.renderTargetWidth = CoordinateTexDesc.Width,
			.renderTargetHeight = CoordinateTexDesc.Height,
			.clearColor = 0x00404040,
			.msaaSampleCount = 0,
			.disableRasterOrdering = false,
			.wireframe = false,
			.fillsDisabled = false,
			.strokesDisabled = false,
		});


		auto renderer = std::make_unique<PLSRenderer>(plsContext.get());

		static StrokeJoin s_join = StrokeJoin::miter;
		static StrokeCap s_cap = StrokeCap::butt;
		static float s_strokeWidth = 70;

		float2 p[9];
		for (int i = 0; i < 9; ++i)
		{
			p[i] = s_pts[i];
		}
		RawPath rawPath;
		rawPath.moveTo(p[0].x, p[0].y);
		rawPath.cubicTo(p[1].x, p[1].y, p[2].x, p[2].y, p[3].x, p[3].y);
		float2 c0 = simd::mix(p[3], p[4], float2(2 / 3.f));
		float2 c1 = simd::mix(p[5], p[4], float2(2 / 3.f));
		rawPath.cubicTo(c0.x, c0.y, c1.x, c1.y, p[5].x, p[5].y);
		rawPath.cubicTo(p[6].x, p[6].y, p[7].x, p[7].y, p[8].x, p[8].y);
		// if (s_doClose)
		//{
		//	rawPath.close();
		// }

		Factory *factory = plsContext.get();
		auto path = factory->makeRenderPath(rawPath, FillRule::nonZero);

		auto fillPaint = factory->makeRenderPaint();
		fillPaint->style(RenderPaintStyle::fill);
		fillPaint->color(-1);

		auto strokePaint = factory->makeRenderPaint();
		strokePaint->style(RenderPaintStyle::stroke);
		strokePaint->color(0x8000ffff);
		strokePaint->thickness(s_strokeWidth);
		strokePaint->join(s_join);
		strokePaint->cap(s_cap);

		renderer->drawPath(path.get(), fillPaint.get());
		renderer->drawPath(path.get(), strokePaint.get());

		// Draw the interactive points.
		auto pointPaint = factory->makeRenderPaint();
		pointPaint->style(RenderPaintStyle::stroke);
		pointPaint->color(0xff0000ff);
		pointPaint->thickness(14);
		pointPaint->cap(StrokeCap::round);

		auto pointPath = factory->makeEmptyRenderPath();
		for (int i : {1, 2, 4, 6, 7})
		{
			float2 pt = s_pts[i];
			pointPath->moveTo(pt.x, pt.y);
		}

		renderer->drawPath(pointPath.get(), pointPaint.get());

		//swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&frameBuffer));
		//renderTarget->setTargetTexture(frameBuffer);
		renderTarget->setTargetTexture(ptex2DCoordinateTexture);
		plsContext->flush({.renderTarget = renderTarget.get()});

#endif

        ///////////////////////////////////////////////////////////////////////////////////////////

        matrix rotateX   = { 1, 0, 0, 0, 0, static_cast<float>(cos(modelRotation.x)), -static_cast<float>(sin(modelRotation.x)), 0, 0, static_cast<float>(sin(modelRotation.x)), static_cast<float>(cos(modelRotation.x)), 0, 0, 0, 0, 1 };
        matrix rotateY   = { static_cast<float>(cos(modelRotation.y)), 0, static_cast<float>(sin(modelRotation.y)), 0, 0, 1, 0, 0, -static_cast<float>(sin(modelRotation.y)), 0, static_cast<float>(cos(modelRotation.y)), 0, 0, 0, 0, 1 };
        matrix rotateZ   = { static_cast<float>(cos(modelRotation.z)), -static_cast<float>(sin(modelRotation.z)), 0, 0, static_cast<float>(sin(modelRotation.z)), static_cast<float>(cos(modelRotation.z)), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        matrix scale     = { modelScale.x, 0, 0, 0, 0, modelScale.y, 0, 0, 0, 0, modelScale.z, 0, 0, 0, 0, 1 };
        matrix translate = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, modelTranslation.x, modelTranslation.y, modelTranslation.z, 1 };

        modelRotation.x += 0.005f;
        modelRotation.y += 0.009f;
        modelRotation.z += 0.001f;

        ///////////////////////////////////////////////////////////////////////////////////////////

        D3D11_MAPPED_SUBRESOURCE mappedSubresource;

        deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);

        Constants* constants = reinterpret_cast<Constants*>(mappedSubresource.pData);

        constants->Transform   = rotateX * rotateY * rotateZ * scale * translate;
        constants->Projection  = { 2 * n / w, 0, 0, 0, 0, 2 * n / h, 0, 0, 0, 0, f / (f - n), 1, 0, 0, n * f / (n - f), 0 };
        constants->LightVector = { 1.0f, -1.0f, 1.0f };

        deviceContext->Unmap(constantBuffer, 0);

        ///////////////////////////////////////////////////////////////////////////////////////////


        deviceContext->ClearRenderTargetView(frameBufferView, backgroundColor);
        deviceContext->ClearDepthStencilView(depthBufferView, D3D11_CLEAR_DEPTH, 1.0f, 0);

        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        deviceContext->IASetInputLayout(inputLayout);
        deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        deviceContext->VSSetShader(vertexShader, nullptr, 0);
        deviceContext->VSSetConstantBuffers(0, 1, &constantBuffer);

        deviceContext->RSSetViewports(1, &viewport);
        deviceContext->RSSetState(rasterizerState);

        deviceContext->PSSetShader(pixelShader, nullptr, 0);
        deviceContext->PSSetShaderResources(0, 1, &textureView);
        deviceContext->PSSetSamplers(0, 1, &samplerState);

        deviceContext->OMSetRenderTargets(1, &frameBufferView, depthBufferView);
        deviceContext->OMSetDepthStencilState(depthStencilState, 0);
        deviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff); // use default blend mode (i.e. disable)

        ///////////////////////////////////////////////////////////////////////////////////////////

        deviceContext->DrawIndexed(ARRAYSIZE(IndexData), 0, 0);

        ///////////////////////////////////////////////////////////////////////////////////////////

        scale = {wasmScale.x, 0, 0, 0, 0, wasmScale.y, 0, 0, 0, 0, wasmScale.z, 0, 0, 0, 0, 1};
        translate = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, wasmTranslation.x, wasmTranslation.y, wasmTranslation.z, 1 };

        deviceContext->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);

        constants = reinterpret_cast<Constants*>(mappedSubresource.pData);

        constants->Transform   = rotateX * rotateY * rotateZ * scale * translate;
        constants->Projection  = { 2 * n / w, 0, 0, 0, 0, 2 * n / h, 0, 0, 0, 0, f / (f - n), 1, 0, 0, n * f / (n - f), 0 };
        constants->LightVector = { 1.0f, -1.0f, 1.0f };

        deviceContext->Unmap(constantBuffer, 0);


		for (auto i = 0; i < tree->Length(); ++i)
		{
			auto node = tree->NodeAt(i);
			if (node->Metadata().find("roof") != std::string::npos)
			{
#ifdef _DEBUG
				deviceContext->PSSetShaderResources(0, 1, &ptex2DCoordinateTextureSRV);
#else
				deviceContext->PSSetShaderResources(0, 1, &textureViewRoof);
#endif
			}
			else if (node->Metadata().find("window") != std::string::npos)
			{
				deviceContext->PSSetShaderResources(0, 1, &textureViewWindow);
			}
			else
			{
				deviceContext->PSSetShaderResources(0, 1, &textureViewWhite);
			}
				
             node->RenderFixedStride(runtime, stride);
		}

        swapChain->Present(1, 0);
#ifdef _DEBUG
        renderTarget->setTargetTexture(nullptr);
#endif
    }

    if (runtime)
		runtime->Release();

    return 0; // PRESS ANY KEY TO EXIT
}

///////////////////////////////////////////////////////////////////////////////////////////////////

matrix operator*(const matrix& m1, const matrix& m2)
{
    return
    {
        m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0],
        m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1],
        m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2],
        m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] + m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3],
        m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0],
        m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1],
        m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2],
        m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] + m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3],
        m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0],
        m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1],
        m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2],
        m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] + m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3],
        m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] + m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0],
        m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] + m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1],
        m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] + m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2],
        m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] + m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3],
    };
}
