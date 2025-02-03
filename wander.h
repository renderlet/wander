#pragma once

#include <string>
#include <vector>

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

enum class BufferFormat
{
	Unknown,
	Custom,
	CustomVertex,
	Index16,
	Index32,
	R8,
	RG8,
	RGB8,
	RGBA8,
	R16F,
	RG16F,
	RGB16F,
	RGBA16F,
	R32F,
	RG32F,
	RGB32F,
	RGBA32F
};

class BufferDescriptor
{
public:
	BufferDescriptor(BufferType type, 
		BufferFormat format = BufferFormat::Custom,
		int width = 0, int height = 0, int depth = 0) :
		m_type(type), m_format(format),
		m_width(width), m_height(height), m_depth(depth)
	{
	}

	BufferType Type() const
	{
		return m_type;
	}

	BufferFormat Format() const
	{
		return m_format;
	}

	int Width() const
	{
		return m_width;
	}

	int Height() const
	{
		return m_height;
	}

	int Depth() const
	{
		return m_depth;
	}

private:
	BufferType m_type;
	BufferFormat m_format;
	int m_width;
	int m_height;
	int m_depth;
};


class IPal : public Object
{
public:
	virtual EPalType Type() = 0;
};

class RenderTreeNode
{
public:
	RenderTreeNode(ObjectID buffer_id, const BufferType& buffer_type, const std::string& metadata, int offset, int length) :
		m_buffer_id(buffer_id), m_buffer_type(buffer_type), m_metadata(metadata),
		m_offset(offset), m_length(length) { }

	void RenderFixedStride(IRuntime* runtime, unsigned int stride) const;

	void RenderVector(IRuntime *runtime, int slot, int width, int height) const;

	void SetPooledBuffer(ObjectID buffer_id, int offset);

	std::string Metadata() const;

	// This should be private
	ObjectID BufferID() const
	{
		return m_buffer_id;
	}

	BufferType Type() const
	{
		return m_buffer_type;
	}

private:
	ObjectID m_buffer_id;
	BufferType m_buffer_type;
	std::string m_metadata;
	int m_offset;
	int m_length;
};


class RenderTree
{
public:
	RenderTree(const std::vector<RenderTreeNode> &nodes) :
		m_nodes(nodes)
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
	virtual ObjectID LoadFromFile(const std::wstring& path) = 0;
	virtual ObjectID LoadFromFile(const std::wstring& path, const std::string& function) = 0;

	virtual void PushParam(ObjectID renderlet_id, float value) = 0;
	virtual void PushParam(ObjectID renderlet_id, double value) = 0;
	virtual void PushParam(ObjectID renderlet_id, uint32_t value) = 0;
	virtual void PushParam(ObjectID renderlet_id, uint64_t value) = 0;
	virtual void ResetStack(ObjectID renderlet_id) = 0;

	virtual ObjectID Render(ObjectID renderlet_id, ObjectID tree_id = -1, bool pool = false) = 0;

	virtual const float* const ExecuteFloat4(ObjectID renderlet_id, const std::string &function) = 0;

	virtual void UploadBufferPool(unsigned int stride) = 0;

	virtual const RenderTree* GetRenderTree(ObjectID tree_id) = 0;
	virtual void DestroyRenderTree(ObjectID tree_id) = 0;

	virtual void Unload(ObjectID renderlet_id) = 0;
};


class Factory
{
public:
	template <typename... ARGs>
	static IPal* CreatePal(EPalType type, ARGs &&...args);

	static IRuntime* CreateRuntime(IPal *pal);
};


}