#include "wander_lib.h"

#include <sstream>

// Required for wasmtime (missing header)
#include <string>
#include "wasmtime.hh"

#include <d3d11.h>
#include <gl3w.c>

#ifdef _WIN32
#pragma comment(lib, "OpenGL32.lib")
#endif

using namespace wander;

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

	if (m_device->CreateBuffer(&vbd, &vinitData, &m_buffers.back()))
		return -1;

	return m_buffers.size() - 1;
}

void PalD3D11::DeleteBuffer(ObjectID buffer_id)
{
	m_buffers[buffer_id]->Release();
}

void PalD3D11::DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned int stride)
{
	const UINT strides = stride;
	constexpr UINT offsets = 0;

	m_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_device_context->IASetVertexBuffers(0, 1, &m_buffers[buffer_id], &strides, &offsets);
	m_device_context->Draw(length, offset);
}

ObjectID PalOpenGL::CreateBuffer(BufferDescriptor desc, int length, uint8_t data[])
{
	switch (desc.Type())
	{
	case BufferType::Vertex:
		break;
	case BufferType::Index:
	case BufferType::Texture2D:
		exit_with_error("PalOpenGL only currently supports BufferType::Vertex",
			nullptr, nullptr);
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

	return 0;
}

void PalOpenGL::DeleteBuffer(ObjectID buffer_id)
{
	glDeleteVertexArrays(1, &m_vaos[buffer_id]);
	glDeleteBuffers(1, &m_vbos[buffer_id]);
}

void PalOpenGL::DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned stride)
{
	glBindVertexArray(m_vaos[buffer_id]);
	glDrawArrays(GL_TRIANGLE_STRIP, offset, length / stride);
	glBindVertexArray(0);
}

void RenderTreeNode::RenderFixedStride(IRuntime* runtime, unsigned int stride) const
{
	static_cast<Runtime*>(runtime)->PalImpl()->DrawTriangleList(m_buffer_id, m_offset, m_length, stride);
}

std::string RenderTreeNode::Metadata() const
{
	return m_metadata;
}

ObjectID wander::Runtime::LoadFromFile(const std::wstring& path)
{
	return LoadFromFile(path, "start");
}

ObjectID wander::Runtime::LoadFromFile(const std::wstring& path, const std::string& function)
{
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

ObjectID wander::Runtime::Render(const ObjectID renderlet_id)
{
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
	auto output = mem + offset;

	BufferDescriptor desc { BufferType::Vertex };

	auto vert_length = *reinterpret_cast<uint32_t *>(output);
	auto verts = output + 4;

	auto id = m_pal->CreateBuffer(desc, vert_length, verts);

	auto mat_length = *reinterpret_cast<uint32_t *>(verts + vert_length);
	auto mats = verts + vert_length + 4;
	auto mat = std::string(mats, mats + mat_length);

	std::vector<RenderTreeNode> nodes;
	

	// TODO - binary going to be more efficient
	std::string line;
	std::istringstream ss(mat);
	while (std::getline(ss, line))
	{
		auto values = split_fixed<4>(',', line);

		int VertexOffset = atoi(values[1].data());
		int VertexLength = atoi(values[2].data());

		auto node = RenderTreeNode{id, line, VertexOffset, VertexLength};

		nodes.push_back(node);
	}

	m_render_trees.push_back(std::make_unique<RenderTree>(nodes));

	return m_render_trees.size() - 1;
}

const RenderTree* wander::Runtime::GetRenderTree(ObjectID tree_id)
{
	return m_render_trees[tree_id].get();
}

void wander::Runtime::DestroyRenderTree(ObjectID tree_id)
{
	for (auto i = 0; i < m_render_trees[tree_id]->Length(); ++i)
	{
		const auto node = m_render_trees[tree_id]->NodeAt(i);
		m_pal->DeleteBuffer(node->BufferID());
	}

	m_render_trees[tree_id]->Clear();
}

void wander::Runtime::Release()
{
	for (auto i = 0; i < m_render_trees.size(); ++i)
	{
		DestroyRenderTree(i);
	}

	m_render_trees.clear();

	for (auto i = 0; i < m_contexts.size(); ++i)
	{
		ResetStack(i);

		wasmtime_module_delete(m_contexts[i].Module);
		wasmtime_store_delete(m_contexts[i].Store);
		wasmtime_linker_delete(m_contexts[i].Linker);
	}

	m_contexts.clear();
	m_params.clear();

	if (m_engine)
	{
		wasm_engine_delete(m_engine);
	}

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
IPal* Factory::CreatePal(EPalType type, ARGs &&...args)
{
	switch (type)
	{
	case EPalType::D3D11:
		return construct<PalD3D11, ARGs...>(std::forward<ARGs>(args)...);
	case EPalType::OpenGL:
	default:
		return construct<PalOpenGL, ARGs...>(std::forward<ARGs>(args)...);
	}
}


IRuntime* Factory::CreateRuntime(IPal *pal)
{
	return new Runtime(static_cast<Pal *>(pal));
}


template class wander::IPal *__cdecl wander::Factory::CreatePal<void *>(enum wander::EPalType, void *&&);

template class wander::IPal *__cdecl wander::Factory::CreatePal<struct ID3D11Device *&, struct ID3D11DeviceContext *&>(
	enum wander::EPalType, struct ID3D11Device *&, struct ID3D11DeviceContext *&);

