#pragma once

#include "wander.h"

#include <ostream>
#include <queue>
#include <utility>

// TODO - this should only be a private dependency
// Required for wasmtime (missing header)
#include <string>
#include "wasmtime.hh"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11Texture2D;

typedef unsigned int GLuint;

namespace wander
{

class Pal : public IPal
{
public:  // TODO: Replace with std::span
	virtual ObjectID CreateBuffer(BufferDescriptor desc, int length, uint8_t data[]) = 0;
	virtual ObjectID CreateTexture(BufferDescriptor desc, int length, uint8_t data[]) = 0;
	virtual void DeleteBuffer(ObjectID buffer_id) = 0;

	virtual void DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned int stride) = 0;
};


class PalD3D11 : public Pal
{
public:
	PalD3D11(ID3D11Device* device, ID3D11DeviceContext* context) :
		m_device(device), m_device_context(context)
	{
	}

	void Release() override{};

	EPalType Type() override
	{
		return EPalType::D3D11;
	}

	ObjectID CreateBuffer(BufferDescriptor desc, int length, uint8_t data[]) override;
	ObjectID CreateTexture(BufferDescriptor desc, int length, uint8_t data[]) override;
	void DeleteBuffer(ObjectID buffer_id) override;

	void DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned int stride) override;

private:
	std::vector<ID3D11Buffer*> m_buffers;
	std::vector<ID3D11Texture2D*> m_textures;

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_device_context;
};


class PalOpenGL : public Pal
{
public:
	PalOpenGL(void* context) : m_context(context) {}

	void Release() override{};

	EPalType Type() override
	{
		return EPalType::OpenGL;
	}

	ObjectID CreateBuffer(BufferDescriptor desc, int length, uint8_t data[]) override;
	ObjectID CreateTexture(BufferDescriptor desc, int length, uint8_t data[]) override;
	void DeleteBuffer(ObjectID buffer_id) override;

	void DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned int stride) override;

private:
	std::vector<GLuint> m_vbos;
	std::vector<GLuint> m_vaos;
	std::vector<GLuint> m_texs;

	void *m_context;
};


class Runtime : public IRuntime
{
public:

	struct Param
	{
		enum
		{
			Int32,
			Int64,
			Float32,
			Float64
		} Type;

		union 
		{
			uint32_t I32;
			uint64_t I64;
			float F32;
			double F64;
		} Value;
	};

	Runtime(Pal *pal) : m_pal(pal)
	{
	}

	ObjectID LoadFromFile(const std::wstring &path) override;
	ObjectID LoadFromFile(const std::wstring &path, const std::string& function) override;

	void PushParam(ObjectID renderlet_id, float value) override;
	void PushParam(ObjectID renderlet_id, double value) override;
	void PushParam(ObjectID renderlet_id, uint32_t value) override;
	void PushParam(ObjectID renderlet_id, uint64_t value) override;
	void ResetStack(ObjectID renderlet_id) override;

	ObjectID Render(ObjectID renderlet_id) override;

	const RenderTree *GetRenderTree(ObjectID tree_id) override;
	void DestroyRenderTree(ObjectID tree_id) override;

	void Release() override;

	Pal* PalImpl() const
	{
		return m_pal;
	}

private:

	struct WasmtimeContext
	{
		wasm_engine_t* Engine = nullptr;
		wasmtime_store_t* Store = nullptr;
		wasmtime_context_t* Context = nullptr;
		wasmtime_linker_t* Linker = nullptr;
		wasmtime_module_t* Module = nullptr;
		wasmtime_extern_t Run {};
		wasmtime_extern_t Memory {};
	};

	wasm_engine_t *m_engine = nullptr;

	std::vector<WasmtimeContext> m_contexts;
	std::vector<std::queue<Param>> m_params;

	std::vector<std::unique_ptr<RenderTree>> m_render_trees;

	Pal* m_pal;
};

}