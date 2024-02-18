#pragma once

#include <memory>
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


class RenderTreeNode
{
public:
	RenderTreeNode(ObjectID buffer_id, const std::string& metadata, int offset, int length) :
		m_buffer_id(buffer_id), m_metadata(metadata),
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
	virtual ObjectID LoadFromFile(const std::wstring &path) = 0;
	virtual ObjectID LoadFromFile(const std::wstring& path, const std::string& function) = 0;

	virtual void PushParam(ObjectID renderlet_id, float value) = 0;
	virtual void PushParam(ObjectID renderlet_id, double value) = 0;
	virtual void PushParam(ObjectID renderlet_id, uint32_t value) = 0;
	virtual void PushParam(ObjectID renderlet_id, uint64_t value) = 0;
	virtual void ResetStack(ObjectID renderlet_id) = 0;

	virtual ObjectID Render(ObjectID renderlet_id) = 0;

	virtual const RenderTree* GetRenderTree(ObjectID tree_id) = 0;
	virtual void DestroyRenderTree(ObjectID tree_id) = 0;
};


class Factory
{
public:
	template <typename... ARGs>
	static IPal* CreatePal(EPalType type, ARGs &&...args);

	static IRuntime* CreateRuntime(IPal *pal);
};


}