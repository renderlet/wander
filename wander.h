#pragma once

#include <utility>
#include <vector>


struct ID3D11Device;

namespace wander
{

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

	BufferType const Type()
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
public:  // TODO" Replace with std::span
	void CreateBuffer(BufferDescriptor desc, int length, uint8_t data[]);
};


class PalD3D11 : public Pal
{
public:
	PalD3D11(ID3D11Device* device) : m_device(device)
	{
	}

	void Release() override{};

	EPalType Type() override
	{
		return EPalType::D3D11;
	}

private:
	ID3D11Device* m_device;
};

typedef int ObjectID;


class RenderTreeNode
{
public:
	void Render();
	std::string Metadata() const;
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
	virtual ObjectID LoadFromFile(std::string path) = 0;

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
	Runtime(Pal *pal);

	ObjectID LoadFromFile(std::string path) override;

	void PushParam(ObjectID renderlet_id, float value) override;
	void PushParam(ObjectID renderlet_id, double value) override;
	void PushParam(ObjectID renderlet_id, uint32_t value) override;
	void PushParam(ObjectID renderlet_id, uint64_t value) override;
	void ResetStack(ObjectID renderlet_id) override;

	ObjectID Render(ObjectID renderlet_id) override;

	const RenderTree *GetRenderTree(ObjectID tree_id) override;
	void DestroyRenderTree(ObjectID tree_id) override;

	void Release() override;
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