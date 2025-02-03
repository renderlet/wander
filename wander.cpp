#ifdef _WIN32
#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
#define NOMINMAX
#endif

#include "wander_lib.h"

#include <array>
#include <cassert>
#include <sstream>
#include <string>
#include <locale>
#include <algorithm>
#include <string_view>


//#include "wasmtime.h"

#ifndef __EMSCRIPTEN__
#include <gl3w.c>
#else
#include <emscripten.h>
#include <GLES3/gl3.h>
#include <GLES3/gl2ext.h>
#include <GLES3/gl3platform.h> 
#endif



#ifdef _WIN32
#include <d3d11.h>
#include <d3d11_1.h>
#pragma comment(lib, "OpenGL32.lib")
#endif

using namespace wander;


#ifndef _WIN32

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <codecvt>

// convert wstring to UTF-8 string
std::string wstring_to_utf8 (const std::wstring& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes(str);
}

int _wfopen_s(FILE **f, const wchar_t *name, const wchar_t *mode) {
    int ret = 0;
    //assert(f);
    *f = fopen(wstring_to_utf8(name).c_str(), wstring_to_utf8(mode).c_str());
    /* Can't be sure about 1-to-1 mapping of errno and MS' errno_t */
    if (!*f)
        ret = errno;
    return ret;
}

#endif

#if defined(RLT_RIVE) && defined(_WIN32)
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

#pragma once

#define SAFE_RELEASE(x)                                                                                                \
	{                                                                                                                  \
		if (x)                                                                                                         \
		{                                                                                                              \
			x->Release();                                                                                              \
			x = 0;                                                                                                     \
		}                                                                                                              \
	}

// used to ensure that TXAA has no aftereffects
class D3D11SavedState
{
public:
	D3D11SavedState(ID3D11DeviceContext *context) : pd3dContext(context)
	{
		pd3dContext->AddRef();
		SaveState();
	}

	D3D11SavedState(ID3D11Device *device)
	{
		device->GetImmediateContext(&pd3dContext);
		SaveState();
	}

	~D3D11SavedState()
	{
		// first set NULL RTs in case we have bounds targets that need to go back to SRVs
		ID3D11RenderTargetView *pNULLRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
		for (int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
			pNULLRTVs[i] = NULL;
		pd3dContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, pNULLRTVs, NULL);

		// Then restore state
		pd3dContext->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pVSSRVs);
		pd3dContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pPSSRVs);
		pd3dContext->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pGSSRVs);
		pd3dContext->HSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pHSSRVs);
		pd3dContext->DSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pDSSRVs);
		pd3dContext->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pCSSRVs);

		UINT NumViewPorts = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
		pd3dContext->RSSetViewports(NumViewPorts, Viewports);
		pd3dContext->VSSetShader(pVertexShader, NULL, NULL);
		pd3dContext->PSSetShader(pPixelShader, NULL, NULL);
		pd3dContext->GSSetShader(pGeometryShader, NULL, NULL);
		pd3dContext->HSSetShader(pHullShader, NULL, NULL);
		pd3dContext->DSSetShader(pDomainShader, NULL, NULL);
		pd3dContext->CSSetShader(pComputeShader, NULL, NULL);

		pd3dContext->VSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pVSSamplers);
		pd3dContext->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pPSSamplers);
		pd3dContext->GSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pGSSamplers);
		pd3dContext->HSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pHSSamplers);
		pd3dContext->DSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pDSSamplers);
		pd3dContext->CSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pCSSamplers);

		pd3dContext->VSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pVSConstantBuffers);
		pd3dContext->PSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pPSConstantBuffers);
		pd3dContext->GSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pGSConstantBuffers);
		pd3dContext->HSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pHSConstantBuffers);
		pd3dContext->DSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pDSConstantBuffers);
		pd3dContext->CSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pCSConstantBuffers);

		pd3dContext->RSSetState(pRasterizer);
		pd3dContext->OMSetDepthStencilState(pDepthStencil, NULL);
		pd3dContext->OMSetBlendState(pBlendState, BlendFactors, BlendSampleMask);
		pd3dContext->IASetInputLayout(pInputLayout);
		pd3dContext->IASetPrimitiveTopology(PrimitiveToplogy);
		// pd3dContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, pRTVs, pDSV,
		// D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, D3D11_PS_CS_UAV_REGISTER_COUNT, pUAVs, 0);
		pd3dContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, pRTVs, pDSV, 0,
															   0, pUAVs, 0);


		SAFE_RELEASE(pd3dContext); // we added a manual ref

		for (int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
			SAFE_RELEASE(pRTVs[i]);
		SAFE_RELEASE(pDSV);
		// for (int i = 0; i < D3D11_PS_CS_UAV_REGISTER_COUNT; i++)
		//	SAFE_RELEASE(pUAVs[i]);


		for (int i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
		{
			SAFE_RELEASE(pVSSRVs[i]);
			SAFE_RELEASE(pPSSRVs[i]);
			SAFE_RELEASE(pGSSRVs[i]);
			SAFE_RELEASE(pHSSRVs[i]);
			SAFE_RELEASE(pDSSRVs[i]);
			SAFE_RELEASE(pCSSRVs[i]);
		}
		SAFE_RELEASE(pVertexShader);
		SAFE_RELEASE(pPixelShader);
		SAFE_RELEASE(pGeometryShader);
		SAFE_RELEASE(pHullShader);
		SAFE_RELEASE(pDomainShader);
		SAFE_RELEASE(pComputeShader);

		for (int i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; i++)
		{
			SAFE_RELEASE(pPSSamplers[i]);
			SAFE_RELEASE(pVSSamplers[i]);
			SAFE_RELEASE(pGSSamplers[i]);
			SAFE_RELEASE(pHSSamplers[i]);
			SAFE_RELEASE(pDSSamplers[i]);
			SAFE_RELEASE(pCSSamplers[i]);
		}
		for (int i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; i++)
		{
			SAFE_RELEASE(pVSConstantBuffers[i]);
			SAFE_RELEASE(pPSConstantBuffers[i]);
			SAFE_RELEASE(pGSConstantBuffers[i]);
			SAFE_RELEASE(pHSConstantBuffers[i]);
			SAFE_RELEASE(pDSConstantBuffers[i]);
			SAFE_RELEASE(pCSConstantBuffers[i]);
		}
		for (int i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; i++)
			SAFE_RELEASE(pPSConstantBuffers[i]);
		SAFE_RELEASE(pRasterizer);
		SAFE_RELEASE(pDepthStencil);
		SAFE_RELEASE(pBlendState);
		SAFE_RELEASE(pInputLayout);
	}

private:
	void SaveState()
	{
		pd3dContext->OMGetRenderTargetsAndUnorderedAccessViews(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, pRTVs, &pDSV, 0,
															   0, pUAVs);
		// pd3dContext->OMGetRenderTargetsAndUnorderedAccessViews(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, pRTVs, &pDSV,
		// D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, D3D11_PS_CS_UAV_REGISTER_COUNT, pUAVs);
		pd3dContext->VSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pVSSRVs);
		pd3dContext->PSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pPSSRVs);
		pd3dContext->GSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pGSSRVs);
		pd3dContext->HSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pHSSRVs);
		pd3dContext->DSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pDSSRVs);
		pd3dContext->CSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, pCSSRVs);

		UINT NumViewPorts = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
		pd3dContext->RSGetViewports(&NumViewPorts, Viewports);
		pd3dContext->VSGetShader(&pVertexShader, NULL, NULL);
		pd3dContext->PSGetShader(&pPixelShader, NULL, NULL);
		pd3dContext->GSGetShader(&pGeometryShader, NULL, NULL);
		pd3dContext->HSGetShader(&pHullShader, NULL, NULL);
		pd3dContext->DSGetShader(&pDomainShader, NULL, NULL);
		pd3dContext->CSGetShader(&pComputeShader, NULL, NULL);

		pd3dContext->VSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pVSSamplers);
		pd3dContext->PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pPSSamplers);
		pd3dContext->GSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pGSSamplers);
		pd3dContext->HSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pHSSamplers);
		pd3dContext->DSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pDSSamplers);
		pd3dContext->CSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, pCSSamplers);

		pd3dContext->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pVSConstantBuffers);
		pd3dContext->PSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pPSConstantBuffers);
		pd3dContext->GSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pGSConstantBuffers);
		pd3dContext->HSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pHSConstantBuffers);
		pd3dContext->DSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pDSConstantBuffers);
		pd3dContext->CSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pCSConstantBuffers);

		pd3dContext->RSGetState(&pRasterizer);
		pd3dContext->OMGetDepthStencilState(&pDepthStencil, NULL);
		pd3dContext->OMGetBlendState(&pBlendState, BlendFactors, &BlendSampleMask);
		pd3dContext->IAGetInputLayout(&pInputLayout);
		pd3dContext->IAGetPrimitiveTopology(&PrimitiveToplogy);
	}

	ID3D11RenderTargetView *pRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	ID3D11UnorderedAccessView *pUAVs[D3D11_PS_CS_UAV_REGISTER_COUNT];
	UINT pUAVInitialCounts[D3D11_PS_CS_UAV_REGISTER_COUNT];

	ID3D11DepthStencilView *pDSV;
	ID3D11ShaderResourceView *pVSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView *pPSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView *pGSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView *pHSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView *pDSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	ID3D11ShaderResourceView *pCSSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];

	D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	ID3D11VertexShader *pVertexShader;
	ID3D11PixelShader *pPixelShader;
	ID3D11GeometryShader *pGeometryShader;
	ID3D11HullShader *pHullShader;
	ID3D11DomainShader *pDomainShader;
	ID3D11ComputeShader *pComputeShader;

	ID3D11SamplerState *pVSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState *pPSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState *pGSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState *pHSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState *pDSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	ID3D11SamplerState *pCSSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];

	ID3D11Buffer *pVSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *pPSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *pGSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *pHSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *pDSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	ID3D11Buffer *pCSConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];

	ID3D11RasterizerState *pRasterizer;
	ID3D11DepthStencilState *pDepthStencil;
	ID3D11BlendState *pBlendState;
	FLOAT BlendFactors[4];
	UINT BlendSampleMask;
	ID3D11InputLayout *pInputLayout;
	D3D11_PRIMITIVE_TOPOLOGY PrimitiveToplogy;

	ID3D11DeviceContext *pd3dContext;
};


#endif

#ifndef __EMSCRIPTEN__

static void exit_with_error(const char *message, wasmtime_error_t *error, wasm_trap_t *trap)
{
	fprintf(stderr, "error: %s\n", message);
	wasm_byte_vec_t error_message;
	if (error != NULL)
	{
		wasmtime_error_message(error, &error_message);
		wasmtime_error_delete(error);
	}
	else
	{
		wasm_trap_message(trap, &error_message);
		wasm_trap_delete(trap);
	}
	fprintf(stderr, "%.*s\n", (int)error_message.size, error_message.data);
	wasm_byte_vec_delete(&error_message);
	exit(1);
}

#endif

template <size_t N>
auto split_fixed(char separator, std::string_view input)
{
	std::array<std::string_view, N> results;
	auto current = input.begin();
	const auto End = input.end();
	for (auto &part : results)
	{
		if (current == End)
		{
			const bool is_last_part = &part == &(results.back());
			if (!is_last_part)
				throw std::invalid_argument("not enough parts to split");
		}
		auto delim = std::find(current, End, separator);
		part = {&*current, size_t(delim - current)};
		current = delim;
		if (delim != End)
			++current;
	}
	if (current != End)
		throw std::invalid_argument("too many parts to split");
	return results;
}

#ifdef _WIN32

PalD3D11::PalD3D11(ID3D11Device *device, ID3D11DeviceContext *context) :
	m_device(device), m_device_context(context), m_swapchain(nullptr)
{
}

PalD3D11::PalD3D11(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapchain) :
	m_device(device), m_device_context(context), m_swapchain(swapchain)
{
#ifdef RLT_RIVE
	PLSRenderContextD3DImpl::ContextOptions contextOptions{};

	m_plsContext = PLSRenderContextD3DImpl::MakeContext(m_device, m_device_context, contextOptions);
#endif
}

PalD3D11::~PalD3D11() {};

ObjectID PalD3D11::CreateBuffer(BufferDescriptor desc, int length, uint8_t data[])
{
	m_buffers.emplace_back(nullptr);

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = length;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = data;

	switch (desc.Type())
	{
	case BufferType::Vertex:
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		break;
	case BufferType::Index:
		vbd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		break;
	case BufferType::Texture2D:
		exit_with_error("Texture2D not supported in D3D11 Buffers", nullptr, nullptr);
		break;
	}

	if (m_device->CreateBuffer(&vbd, &vinitData, &m_buffers.back()) != 0)
		return -1;

	return m_buffers.size() - 1;
}

ObjectID PalD3D11::CreateTexture(BufferDescriptor desc, int length, uint8_t data[])
{
	m_textures.emplace_back(nullptr);

	DXGI_FORMAT format{};
	auto pitch = 0;

	switch (desc.Format())
	{
	case BufferFormat::Unknown:
	case BufferFormat::Custom:
	case BufferFormat::CustomVertex:
	case BufferFormat::Index16:
	case BufferFormat::Index32:
	default:
		return -1;
	case BufferFormat::R8:
		format = DXGI_FORMAT_R8_UNORM;
		pitch = 1;
		break;
	case BufferFormat::RG8:
		format = DXGI_FORMAT_R8G8_UNORM;
		pitch = 2;
		break;
	case BufferFormat::RGB8:
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
		pitch = 3; // legacy
		break;
	case BufferFormat::RGBA8:
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
		pitch = 4;
		break;
	case BufferFormat::R16F:
		format = DXGI_FORMAT_R16_FLOAT;
		pitch = 2;
		break;
	case BufferFormat::RG16F:
		format = DXGI_FORMAT_R16G16_FLOAT;
		pitch = 4;
		break;
	case BufferFormat::RGB16F:
		format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		pitch = 6;  // legacy
		break;
	case BufferFormat::RGBA16F:
		format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		pitch = 8;
		break;
	case BufferFormat::R32F:
		format = DXGI_FORMAT_R32_FLOAT;
		pitch = 4;
		break;
	case BufferFormat::RG32F:
		format = DXGI_FORMAT_R32G32_FLOAT;
		pitch = 8;
		break;
	case BufferFormat::RGB32F:
		format = DXGI_FORMAT_R32G32B32_FLOAT;
		pitch = 12;
		break;
	case BufferFormat::RGBA32F:
		format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		pitch = 16;
		break;
	}

	// Copy into a texture.
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = desc.Width();
	texDesc.Height = desc.Height();
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA sub;
	sub.pSysMem = data;
	sub.SysMemPitch = pitch;
	sub.SysMemSlicePitch = 0; // no array support yet

	if (m_device->CreateTexture2D(&texDesc, &sub, &m_textures.back()))
		return -1;

	return m_textures.size() - 1;
}

void PalD3D11::DeleteBuffer(ObjectID buffer_id)
{
	if (buffer_id != -1)
		m_buffers[buffer_id]->Release();
}

ObjectID PalD3D11::CreateVector(int length, uint8_t data[])
{
#ifdef RLT_RIVE
	m_vector_commands.emplace_back(VectorLoader::Read(data, length));

	m_vector_srvs.emplace_back(nullptr);
	m_vector_textures.emplace_back(nullptr);

	return m_vector_commands.size() - 1;
#else
	return -1;
#endif
}

ObjectID PalD3D11::UpdateVector(int length, uint8_t data[], ObjectID buffer_id)
{
#ifdef RLT_RIVE
	m_vector_commands[buffer_id] = VectorLoader::Read(data, length);
	return buffer_id;
#else
	return -1;
#endif
}

void PalD3D11::DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned int stride)
{
	if (buffer_id < 0)
		return;

	const UINT strides = stride;
	constexpr UINT offsets = 0;

	m_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_device_context->IASetVertexBuffers(0, 1, &m_buffers[buffer_id], &strides, &offsets);
	m_device_context->Draw(length, offset);
}

void PalD3D11::DrawVector(ObjectID buffer_id, int slot, int width, int height)
{
#if defined(RLT_RIVE)
	{
		D3D11SavedState savedState(m_device_context);

		const auto &vector = m_vector_commands[buffer_id];

		rive::Factory *factory = m_plsContext.get();

		auto renderer = std::make_unique<PLSRenderer>(m_plsContext.get());

		auto plsContextImpl = m_plsContext->static_impl_cast<PLSRenderContextD3DImpl>();

		auto renderTarget = plsContextImpl->makeRenderTarget(width, height);

		if (slot != -1 && m_vector_textures[buffer_id] == nullptr)
		{
			D3D11_TEXTURE2D_DESC CoordinateTexDesc = {
				static_cast<UINT>(width), // UINT Width;
				static_cast<UINT>(height), // UINT Height;
				1, // UINT MipLevels;
				1, // UINT ArraySize;
				DXGI_FORMAT_R8G8B8A8_UNORM, // DXGI_FORMAT Format;
				{1, 0}, // DXGI_SAMPLE_DESC SampleDesc;
				D3D11_USAGE_DEFAULT, // D3D11_USAGE Usage;
				D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, // UINT BindFlags;
				0, // UINT CPUAccessFlags;
				0, // UINT MiscFlags;
			};

			m_device->CreateTexture2D(&CoordinateTexDesc, NULL, &m_vector_textures[buffer_id]);
			m_device->CreateShaderResourceView(m_vector_textures[buffer_id], NULL, &m_vector_srvs[buffer_id]);
		}

		m_plsContext->beginFrame({
			.renderTargetWidth = static_cast<uint32_t>(width),
			.renderTargetHeight = static_cast<uint32_t>(height),
			.clearColor = 0xFF000000,
			.msaaSampleCount = 0,
			.disableRasterOrdering = false,
			.wireframe = false,
			.fillsDisabled = false,
			.strokesDisabled = false,
		});

		if (slot == -1)
		{
			ID3D11Texture2D *frameBuffer;

			m_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&frameBuffer));

			renderTarget->setTargetTexture(frameBuffer);
		}
		else
		{
			renderTarget->setTargetTexture(m_vector_textures[buffer_id]);
		}

		for (const auto &paths : vector)
		{
			const auto &raw_paint = std::get<0>(paths);
			auto paint = factory->makeRenderPaint();

			//paint->blendMode(static_cast<BlendMode>(raw_paint.blendMode));
			paint->join(static_cast<StrokeJoin>(raw_paint.join));
			paint->cap(static_cast<StrokeCap>(raw_paint.cap));
			paint->style(static_cast<RenderPaintStyle>(raw_paint.style));
			paint->thickness(raw_paint.thickness);
			paint->color(raw_paint.color);

			RawPath rp{};
			const auto &raw_path = std::get<1>(paths);

			for (const auto &path : raw_path)
			{
				const auto type = std::get<0>(path);
				const auto &points = std::get<1>(path);

				switch (type)
				{
				case VectorLoader::EPathType::Move:
					rp.moveTo(points[0].x, points[0].y);
					break;
				case VectorLoader::EPathType::Line:
					rp.lineTo(points[0].x, points[0].y);
					break;
				case VectorLoader::EPathType::Quad:
					rp.quadTo(points[0].x, points[0].y, points[1].x, points[1].y);
					break;
				case VectorLoader::EPathType::Cubic:
					rp.cubicTo(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
					break;
				case VectorLoader::EPathType::Close:
					rp.close();
					break;
				}
			}

			auto path = factory->makeRenderPath(rp, FillRule::nonZero);

			renderer->drawPath(path.get(), paint.get());

			
		}
		m_plsContext->flush({.renderTarget = renderTarget.get()});
	}
	if (slot != -1)
		m_device_context->PSSetShaderResources(slot, 1, &m_vector_srvs[buffer_id]);
#endif
}

#endif

ObjectID PalOpenGL::CreateBuffer(BufferDescriptor desc, int length, uint8_t data[])
{
	switch (desc.Type())
	{
	case BufferType::Vertex:
		break;
	case BufferType::Index:
	case BufferType::Texture2D:
		CreateTexture(desc, length, data);
		break;
	}

	GLuint vbo{};
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, length, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	m_vbos.emplace_back(vbo);

	GLuint vao{};
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	m_vaos.emplace_back(vao);

	return m_vaos.size() - 1;
}

ObjectID PalOpenGL::CreateTexture(BufferDescriptor desc, int length, uint8_t data[])
{
	GLenum format{};
	GLenum type{};

	switch (desc.Format())
	{
	case BufferFormat::Unknown:
	case BufferFormat::Custom:
	case BufferFormat::CustomVertex:
	case BufferFormat::Index16:
	case BufferFormat::Index32:
		return -1;
	case BufferFormat::R8:
		format = GL_UNSIGNED_BYTE;
		type = GL_RED;
		break;
	case BufferFormat::RG8:
		format = GL_UNSIGNED_BYTE;
		type = GL_RG;
		break;
	case BufferFormat::RGB8:
		format = GL_UNSIGNED_BYTE;
		type = GL_RGB;
		break;
	case BufferFormat::RGBA8:
		format = GL_UNSIGNED_BYTE;
		type = GL_RGBA;
		break;
	case BufferFormat::R16F:
		format = GL_HALF_FLOAT;
		type = GL_RED;
		break;
	case BufferFormat::RG16F:
		format = GL_HALF_FLOAT;
		type = GL_RG;
		break;
	case BufferFormat::RGB16F:
		format = GL_HALF_FLOAT;
		type = GL_RGB;
		break;
	case BufferFormat::RGBA16F:
		format = GL_HALF_FLOAT;
		type = GL_RGBA;
		break;
	case BufferFormat::R32F:
		format = GL_FLOAT;
		type = GL_RED;
		break;
	case BufferFormat::RG32F:
		format = GL_FLOAT;
		type = GL_RG;
		break;
	case BufferFormat::RGB32F:
		format = GL_FLOAT;
		type = GL_RGB;
		break;
	case BufferFormat::RGBA32F:
		format = GL_FLOAT;
		type = GL_RGBA;
		break;
	default:
		return -1;
	}

	// don't (yet) support tex arrays
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, type, 
		desc.Width(), desc.Height(), 0, type, format, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	m_texs.emplace_back(texture);

	return m_texs.size() - 1;
}

void PalOpenGL::DeleteBuffer(ObjectID buffer_id)
{
	glDeleteVertexArrays(1, &m_vaos[buffer_id]);
	glDeleteBuffers(1, &m_vbos[buffer_id]);
}

ObjectID PalOpenGL::CreateVector(int length, uint8_t data[])
{
	return -1;
}

ObjectID PalOpenGL::UpdateVector(int length, uint8_t data[], ObjectID buffer_id)
{
	return -1;
}

void wander::PalOpenGL::DrawVector(ObjectID buffer_id, int slot, int width, int height)
{

}

void PalOpenGL::DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned stride)
{
	glBindVertexArray(m_vaos[buffer_id]);
	glDrawArrays(GL_TRIANGLES, offset, length);
	glBindVertexArray(0);
}

void RenderTreeNode::RenderFixedStride(IRuntime* runtime, unsigned int stride) const
{
	static_cast<Runtime*>(runtime)->PalImpl()->DrawTriangleList(m_buffer_id, m_offset, m_length, stride);
}

void RenderTreeNode::RenderVector(IRuntime *runtime, int slot, int width, int height) const
{
	static_cast<Runtime*>(runtime)->PalImpl()->DrawVector(m_buffer_id, slot, width, height);
}

void RenderTreeNode::SetPooledBuffer(ObjectID buffer_id, int offset)
{
	m_buffer_id = buffer_id;
	m_offset += offset;
}

std::string RenderTreeNode::Metadata() const
{
	return m_metadata;
}

#ifdef __EMSCRIPTEN__

EM_ASYNC_JS(uintptr_t, init_renderlet, (const char *str), {
	if (Window.renderlet_id === undefined){
		Window.renderlet_id = 0;
	}
	else {
		++Window.renderlet_id;
	}
	
	await WebAssembly.instantiateStreaming(fetch(UTF8ToString(str)), {}).then(function(obj) {
		Window["Start_" + Window.renderlet_id] = obj.instance.exports.Start;
		Window["Start_mem_" + Window.renderlet_id] = obj.instance.exports.memory;
	});
	return Window.renderlet_id;
});


EM_ASYNC_JS(uintptr_t, run_renderlet, (int id), {
	var offset = Window["Start_" + id]();

	var length_output = new Uint8Array(Window["Start_mem_" + id].buffer, offset, 4);
	var view = new DataView(length_output.buffer, offset, 4);
	var length = view.getUint32(0, true);

	var output = new Uint8Array(Window["Start_mem_" + id].buffer, offset, length);

	var heapSpace = _malloc(output.length);
	HEAP8.set(output, heapSpace);
	return heapSpace;
});

EM_ASYNC_JS(void, delete_renderlet, (int id), {
	Window["Start_" + id] = {};
	Window["Start_mem_" + id] = {};
});

#endif

ObjectID wander::Runtime::LoadFromFile(const std::wstring& path)
{
	return LoadFromFile(path, "Start");
}

ObjectID wander::Runtime::LoadFromFile(const std::wstring& path, const std::string& function)
{
#ifndef __EMSCRIPTEN__

	auto context = WasmtimeContext{};

	auto conf = wasm_config_new();
	wasmtime_config_wasm_simd_set(conf, true);
	wasmtime_config_wasm_bulk_memory_set(conf, true);
	wasmtime_config_wasm_multi_value_set(conf, true);
	wasmtime_config_wasm_multi_memory_set(conf, true);
	wasmtime_config_wasm_reference_types_set(conf, true);
	wasmtime_config_wasm_threads_set(conf, true);
	wasmtime_config_cranelift_opt_level_set(conf, WASMTIME_OPT_LEVEL_NONE);
	wasmtime_config_parallel_compilation_set(conf, true);
	wasmtime_config_cranelift_debug_verifier_set(conf, false);

	m_engine = wasm_engine_new_with_config(conf);
	assert(m_engine != NULL);
	context.Store = wasmtime_store_new(m_engine, NULL, NULL);
	assert(context.Store != NULL);
	context.Context = wasmtime_store_context(context.Store);

	// Create a linker with WASI functions defined
	context.Linker = wasmtime_linker_new(m_engine);
	wasmtime_error_t *error = wasmtime_linker_define_wasi(context.Linker);
	if (error != NULL)
		exit_with_error("failed to link wasi", error, NULL);

	wasm_byte_vec_t wasm;
	// Load our input file to parse it next
	FILE *file;
	if (_wfopen_s(&file, path.c_str(), L"rb") || !file)
	{
		printf("> Error loading file!\n");
		exit(1);
	}
	fseek(file, 0L, SEEK_END);
	size_t file_size = ftell(file);
	wasm_byte_vec_new_uninitialized(&wasm, file_size);
	fseek(file, 0L, SEEK_SET);
	if (fread(wasm.data, file_size, 1, file) != 1)
	{
		printf("> Error loading module!\n");
		exit(1);
	}
	fclose(file);

	error = wasmtime_module_new(m_engine, (uint8_t *)wasm.data, wasm.size, &context.Module);
	if (!context.Module)
		exit_with_error("failed to compile module", error, NULL);
	wasm_byte_vec_delete(&wasm);


	// Instantiate wasi
	wasi_config_t *wasi_config = wasi_config_new();
	assert(wasi_config);
	wasi_config_inherit_argv(wasi_config);
	wasi_config_inherit_env(wasi_config);
	wasi_config_inherit_stdin(wasi_config);
	wasi_config_inherit_stdout(wasi_config);
	wasi_config_inherit_stderr(wasi_config);

	error = wasmtime_context_set_wasi(context.Context, wasi_config);
	if (error != NULL)
		exit_with_error("failed to instantiate WASI", error, NULL);

	error = wasmtime_linker_module(context.Linker, context.Context, "", 0, context.Module);
	if (error != NULL)
		exit_with_error("failed to instantiate module", error, NULL);

	if (!wasmtime_linker_get(context.Linker, context.Context, "", 0, 
		function.c_str(), function.length(), &context.Run))
		return -1;

	if (!wasmtime_linker_get(context.Linker, context.Context, "", 0, 
		"memory", 6, &context.Memory))
		return -1;

	m_contexts.push_back(context);

	m_params.push_back({});

	return m_contexts.size() - 1;

#else

	//init_renderlet(path.c_str());
	m_context_count = init_renderlet("demo.wasm") + 1;

	return m_context_count - 1;
#endif
}

void wander::Runtime::PushParam(ObjectID renderlet_id, float value)
{
	Param p;
	p.Type = Param::Float32;
	p.Value.F32 = value;

	m_params[renderlet_id].push(p);
}

void wander::Runtime::PushParam(ObjectID renderlet_id, double value)
{
	Param p;
	p.Type = Param::Float64;
	p.Value.F64 = value;

	m_params[renderlet_id].push(p);
}

void wander::Runtime::PushParam(ObjectID renderlet_id, uint32_t value)
{
	Param p;
	p.Type = Param::Int32;
	p.Value.I32 = value;

	m_params[renderlet_id].push(p);
}

void wander::Runtime::PushParam(ObjectID renderlet_id, uint64_t value)
{
	Param p;
	p.Type = Param::Int64;
	p.Value.I64 = value;

	m_params[renderlet_id].push(p);
}

void wander::Runtime::ResetStack(ObjectID renderlet_id)
{
	m_params[renderlet_id] = {};
}

ObjectID wander::Runtime::BuildVector(uint32_t length, uint8_t *data, ObjectID tree_id)
{
	if (tree_id != -1)
	{
		auto buffer_id = m_render_trees[tree_id]->NodeAt(0)->BufferID();

		m_pal->UpdateVector(length, data, buffer_id);

		return tree_id;
	}

	std::vector<RenderTreeNode> nodes;

	auto id = m_pal->CreateVector(length, data);

	auto node = RenderTreeNode{id, BufferType::Texture2D, "", 0, 0};

	nodes.push_back(node);

	m_render_trees.push_back(std::make_unique<RenderTree>(nodes));
	return m_render_trees.size() - 1;
}

void Runtime::CreatePooledBuffer(uint32_t length, uint8_t *data, ObjectID tree_id)
{
	auto offset = 0;
	if (m_staging_buffer == nullptr)
	{
		m_staging_buffer = std::make_unique<uint8_t[]>(600 * 1024 * 1024); // MB
	}
	else
	{
		const auto& sub = m_sub_buffers.back();
		offset = sub.offset + sub.length;
	}

	memcpy(&m_staging_buffer[offset], data, length);

	m_sub_buffers.emplace_back(
		SubBuffer {
			offset,
			length,
			tree_id
		}
	);
}

ObjectID Runtime::Render(const ObjectID renderlet_id, ObjectID tree_id, bool pool)
{
#ifndef __EMSCRIPTEN__

	std::vector<wasmtime_val_t> args(m_params[renderlet_id].size());

	for (auto& [kind, of] : args)
	{
		switch (const auto [Type, Value] = m_params[renderlet_id].front(); Type)
		{
		case Param::Int32:
			kind = WASMTIME_I32;
			of.i32 = Value.I32;
			break;
		case Param::Int64:
			kind = WASMTIME_I64;
			of.i64 = Value.I64;
			break;
		case Param::Float32:
			kind = WASMTIME_F32;
			of.f32 = Value.F32;
			break;
		case Param::Float64:
			kind = WASMTIME_F64;
			of.f64 = Value.F64;
			break;
		}

		m_params[renderlet_id].pop();
	}

	auto &context = m_contexts[renderlet_id];

	wasmtime_val_t results[1];

	
	wasm_trap_t *trap = nullptr;
	wasmtime_error_t *error =
		wasmtime_func_call(context.Context, &context.Run.of.func, args.data(), args.size(), results, 1, &trap);

	if (error != NULL || trap != NULL)
		exit_with_error("failed to call run", error, trap);

	auto mem = wasmtime_memory_data(context.Context, &context.Memory.of.memory);
	auto offset = results[0].of.i32; 

	// CreateBuffer with this
	auto output = mem + offset + 4;

#else
	auto value = run_renderlet(renderlet_id);

	uint8_t *output = reinterpret_cast<uint8_t *>(value + 4);

#endif

	BufferDescriptor desc { BufferType::Vertex };

	auto version = *reinterpret_cast<uint32_t *>(output);
	if (version != 1)
	{
		return -1;
	}

	auto vert_length = *reinterpret_cast<uint32_t *>(output + sizeof(uint32_t));
	auto vert_format = *reinterpret_cast<uint32_t *>(output + 2 * sizeof(uint32_t));
	auto verts = output + 3 * sizeof(uint32_t);

	if (vert_format == 3)
	{
		desc = {BufferType::Index};
	}

	if (vert_format == 2)
	{
		return BuildVector(vert_length, verts, tree_id);
	}

	auto id = pool ? -1 : m_pal->CreateBuffer(desc, vert_length, verts);

	auto mat_length = *reinterpret_cast<uint32_t *>(verts + vert_length);
	auto mats = verts + vert_length + 4;
	auto mat = std::string(mats, mats + mat_length);

	std::vector<RenderTreeNode> nodes;

	// TODO - binary going to be more efficient
	std::string line;
	std::istringstream ss(mat);
	while (std::getline(ss, line))
	{
		auto values = split_fixed<5>(',', line);

		int VertexOffset = atoi(values[1].data());
		int VertexLength = atoi(values[2].data());

		auto node = RenderTreeNode{id, BufferType::Vertex, line, VertexOffset, VertexLength};

		nodes.push_back(node);
	}
	
	m_render_trees.push_back(std::make_unique<RenderTree>(nodes));

	if (pool)
	{
		CreatePooledBuffer(vert_length, verts, m_render_trees.size() - 1);
	}

	return m_render_trees.size() - 1;
}

const float* const Runtime::ExecuteFloat4(ObjectID renderlet_id, const std::string& function)
{
	std::vector<wasmtime_val_t> args(m_params[renderlet_id].size());

	for (auto &[kind, of] : args)
	{
		switch (const auto [Type, Value] = m_params[renderlet_id].front(); Type)
		{
		case Param::Int32:
			kind = WASMTIME_I32;
			of.i32 = Value.I32;
			break;
		case Param::Int64:
			kind = WASMTIME_I64;
			of.i64 = Value.I64;
			break;
		case Param::Float32:
			kind = WASMTIME_F32;
			of.f32 = Value.F32;
			break;
		case Param::Float64:
			kind = WASMTIME_F64;
			of.f64 = Value.F64;
			break;
		}

		m_params[renderlet_id].pop();
	}

	auto& context = m_contexts[renderlet_id];

	wasmtime_val_t results[1];

	wasmtime_extern_t expression{};

	if (!wasmtime_linker_get(context.Linker, context.Context, "", 0, 
		function.c_str(), function.length(), &expression))
		return nullptr;

	wasm_trap_t *trap = nullptr;
	wasmtime_error_t *error =
		wasmtime_func_call(context.Context, &expression.of.func, args.data(), args.size(), results, 1, &trap);

	if (error != NULL || trap != NULL)
		exit_with_error("failed to call expression", error, trap);

	auto mem = wasmtime_memory_data(context.Context, &context.Memory.of.memory);
	auto offset = results[0].of.i32;

	return reinterpret_cast<float*>(mem + offset);
}


void Runtime::UploadBufferPool(unsigned int stride)
{
	const auto& last = m_sub_buffers.back();
	const auto length = last.offset + last.length;

	const BufferDescriptor desc{BufferType::Vertex};
	const auto id = m_pal->CreateBuffer(desc, length, m_staging_buffer.get());

	auto offset = 0;

	for (const auto& sub: m_sub_buffers)
	{
		const auto* tree = m_render_trees[sub.tree_id].get();

		for (auto i = 0; i < tree->Length(); ++i)
		{
			const_cast<RenderTreeNode*>(tree->NodeAt(i))->SetPooledBuffer(id, offset);
		}

		offset += sub.length / stride;
	}

	m_staging_buffer.reset(nullptr);
	m_sub_buffers.clear();
}

const RenderTree *wander::Runtime::GetRenderTree(ObjectID tree_id)
{
	return m_render_trees[tree_id].get();
}

void wander::Runtime::DestroyRenderTree(ObjectID tree_id)
{
	for (auto i = 0; i < m_render_trees[tree_id]->Length(); ++i)
	{
		const auto node = m_render_trees[tree_id]->NodeAt(i);
		if (node->Type() == BufferType::Vertex)
			m_pal->DeleteBuffer(node->BufferID());
		// TODO - other resource types
	}

	m_render_trees[tree_id]->Clear();
}

void wander::Runtime::Unload(ObjectID renderlet_id)
{
	if (renderlet_id >= 0 && m_contexts[renderlet_id].Module != nullptr)
	{
		ResetStack(renderlet_id);
		wasmtime_module_delete(m_contexts[renderlet_id].Module);
		wasmtime_store_delete(m_contexts[renderlet_id].Store);
		wasmtime_linker_delete(m_contexts[renderlet_id].Linker);
		m_contexts[renderlet_id].Module = nullptr;
		m_contexts[renderlet_id].Store = nullptr;
		m_contexts[renderlet_id].Linker = nullptr;
	}
}

void wander::Runtime::Release()
{
#ifndef __EMSCRIPTEN__
	for (auto i = 0; i < m_render_trees.size(); ++i)
	{
		DestroyRenderTree(i);
	}

	m_render_trees.clear();

	for (auto i = 0; i < m_contexts.size(); ++i)
	{
		Unload(i);
	}

	m_contexts.clear();
	m_params.clear();

	if (m_engine)
	{
		wasm_engine_delete(m_engine);
	}
#else
	for (auto i = 0; i < m_context_count; ++i)
	{
		delete_renderlet(i);
	}
#endif
	m_pal->Release();
}

template <class T, class... Args,
	std::enable_if_t<!std::is_constructible<T, Args &&...>::value, bool> = true>
 // Using helper type
T* construct(Args &&...args)
{
	return nullptr;
}	

template <class T, class... Args,
	std::enable_if_t<std::is_constructible<T, Args &&...>::value, bool> = true>
T* construct(Args &&...args)
{
	return new T(std::forward<Args>(args)...);
}

template <typename... ARGs>
IPal* wander::Factory::CreatePal(EPalType type, ARGs &&...args)
{
	switch (type)
	{
#ifdef _WIN32
	case EPalType::D3D11:
		return construct<PalD3D11, ARGs...>(std::forward<ARGs>(args)...);
#endif
	case EPalType::OpenGL:
	default:
		return construct<PalOpenGL, ARGs...>(std::forward<ARGs>(args)...);
	}
}


IRuntime* wander::Factory::CreateRuntime(IPal *pal)
{
	return new Runtime(static_cast<Pal *>(pal));
}


template class wander::IPal *__cdecl wander::Factory::CreatePal<void *>(enum wander::EPalType, void *&&);

#ifdef _WIN32

template class wander::IPal *__cdecl wander::Factory::CreatePal<struct ID3D11Device *&, struct ID3D11DeviceContext *&>
	(enum wander::EPalType, struct ID3D11Device *&, struct ID3D11DeviceContext *&);

template class wander::IPal *__cdecl wander::Factory::CreatePal<struct ID3D11Device *&, struct ID3D11DeviceContext *&,
	struct IDXGISwapChain *&>(enum wander::EPalType, struct ID3D11Device *&, struct ID3D11DeviceContext *&, struct IDXGISwapChain *&);
#endif

CByteArrayStreambuf::CByteArrayStreambuf(const uint8_t *begin, const size_t size) :
	m_begin(begin), m_end(begin + size), m_current(m_begin)
{
	assert(std::less_equal<>()(m_begin, m_end));
}


CByteArrayStreambuf::int_type CByteArrayStreambuf::underflow()
{
	if (m_current == m_end)
		return traits_type::eof();

	return traits_type::to_int_type(*m_current);
}


CByteArrayStreambuf::int_type CByteArrayStreambuf::uflow()
{
	if (m_current == m_end)
		return traits_type::eof();

	return traits_type::to_int_type(*m_current++);
}


CByteArrayStreambuf::int_type CByteArrayStreambuf::pbackfail(int_type ch)
{
	if (m_current == m_begin || (ch != traits_type::eof() && ch != m_current[-1]))
		return traits_type::eof();

	return traits_type::to_int_type(*--m_current);
}


std::streamsize CByteArrayStreambuf::showmanyc()
{
	assert(std::less_equal<>()(m_current, m_end));
	return m_end - m_current;
}


std::streampos CByteArrayStreambuf::seekoff(std::streamoff off, std::ios_base::seekdir way,
											std::ios_base::openmode which)
{
	if (way == std::ios_base::beg)
	{
		m_current = m_begin + off;
	}
	else if (way == std::ios_base::cur)
	{
		m_current += off;
	}
	else if (way == std::ios_base::end)
	{
		m_current = m_end;
	}

	if (m_current < m_begin || m_current > m_end)
		return -1;

	return m_current - m_begin;
}


std::streampos CByteArrayStreambuf::seekpos(std::streampos sp, std::ios_base::openmode which)
{
	m_current = m_begin + sp;

	if (m_current < m_begin || m_current > m_end)
		return -1;

	return m_current - m_begin;
}
