#pragma once

#include <queue>
#include <utility>
#include <vector>

// TODO - this should only be a private dependency
// Required for wasmtime (missing header)
#include <string>
#include "wasmtime.hh"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;

namespace wander
{
	class IRuntime;

	typedef int ObjectID;

enum class EPalType
{
	D3D11,
	OpenGL
};

class Object
{
public:
	virtual void Release() = 0;
};


enum class BufferType
{
	Vertex,
	Index,
	Texture2D
};

class BufferDescriptor
{
public:
	BufferDescriptor(BufferType type) : m_type(type)
	{
	}

	BufferType Type() const
	{
		return m_type;
	}

private:
	BufferType m_type;
};


class IPal : public Object
{
public:
	virtual EPalType Type() = 0;
};


class Pal : public IPal
{
public:  // TODO: Replace with std::span
	virtual ObjectID CreateBuffer(BufferDescriptor desc, int length, uint8_t data[]) = 0;
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
	void DeleteBuffer(ObjectID buffer_id) override;

	void DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned int stride) override;

private:
	std::vector<ID3D11Buffer*> m_buffers;

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_device_context;
};


class RenderTreeNode
{
public:
	RenderTreeNode(ObjectID buffer_id, std::string metadata, int offset, int length) :
		m_buffer_id(buffer_id), m_metadata(std::move(metadata)),
		m_offset(offset), m_length(length) { }

	void RenderFixedStride(IRuntime* runtime, unsigned int stride) const;

	std::string Metadata() const;

	// This should be private
	ObjectID BufferID() const
	{
		return m_buffer_id;
	}

private:
	ObjectID m_buffer_id;
	std::string m_metadata;
	int m_offset;
	int m_length;
};


class RenderTree
{
public:
	RenderTree(std::vector<RenderTreeNode> nodes) :
		m_nodes(std::move(nodes))
	{

	}

	const RenderTreeNode* NodeAt(int index) const
	{
		return &m_nodes[index]; // bounds check
	}

	void Clear()
	{
		m_nodes.clear();
	}

	int Length() const
	{
		return m_nodes.size();
	}

private:
	std::vector<RenderTreeNode> m_nodes;
};



class IRuntime : public Object
{
public:
	virtual ObjectID LoadFromFile(std::wstring path) = 0;

	virtual void PushParam(ObjectID renderlet_id, float value) = 0;
	virtual void PushParam(ObjectID renderlet_id, double value) = 0;
	virtual void PushParam(ObjectID renderlet_id, uint32_t value) = 0;
	virtual void PushParam(ObjectID renderlet_id, uint64_t value) = 0;
	virtual void ResetStack(ObjectID renderlet_id) = 0;

	virtual ObjectID Render(ObjectID renderlet_id) = 0;

	virtual const RenderTree* GetRenderTree(ObjectID tree_id) = 0;
	virtual void DestroyRenderTree(ObjectID tree_id) = 0;
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

	ObjectID LoadFromFile(std::wstring path) override;

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

	std::vector<RenderTree> m_render_trees;

	Pal* m_pal;
};

class Factory
{
public:
	template <typename... ARGs>
	static IPal* CreatePal(EPalType type, ARGs &&...args)
	{
		switch (type)
		{
		case EPalType::D3D11:
			return new PalD3D11(std::forward<ARGs>(args)...);
		}

	}

	static IRuntime* CreateRuntime(IPal *pal)
	{
		return new Runtime(static_cast<Pal*>(pal));
	}
};


}