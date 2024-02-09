#include "wander.h"

using namespace wander;

ObjectID wander::Runtime::LoadFromFile(std::string path)
{
	return ObjectID();
}

void wander::Runtime::PushParam(ObjectID renderlet_id, float value)
{
}

void wander::Runtime::PushParam(ObjectID renderlet_id, double value)
{
}

void wander::Runtime::PushParam(ObjectID renderlet_id, uint32_t value)
{
}

void wander::Runtime::PushParam(ObjectID renderlet_id, uint64_t value)
{
}

void wander::Runtime::ResetStack(ObjectID renderlet_id)
{
}

ObjectID wander::Runtime::Render(ObjectID renderlet_id)
{
	return ObjectID();
}

const RenderTree *wander::Runtime::GetRenderTree(ObjectID tree_id)
{
	return nullptr;
}

void wander::Runtime::DestroyRenderTree(ObjectID tree_id)
{
}

void wander::Runtime::Release()
{
}
