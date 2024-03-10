// OBJHost.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>

// Required for wasmtime (missing header)
#include <string>
#include <vector>
#include "wasmtime.h"


#ifdef _WIN64
// TODO - move to wander lib
#pragma comment(lib, "wasmtime.dll.lib")
#endif

int fopen_s(FILE **f, const char *name, const char *mode) {
    int ret = 0;
    assert(f);
    *f = fopen(name, mode);
    /* Can't be sure about 1-to-1 mapping of errno and MS' errno_t */
    if (!*f)
        ret = errno;
    return ret;
}

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

int main(int argc, char **argv)
{
	std::string inFile = "";
	if (argc == 2)
	{
		inFile = argv[1];
	}

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

	auto m_engine = wasm_engine_new_with_config(conf);
	assert(m_engine != NULL);
	auto Store = wasmtime_store_new(m_engine, NULL, NULL);
	assert(Store != NULL);
	auto Context = wasmtime_store_context(Store);

	// Create a linker with WASI functions defined
	auto Linker = wasmtime_linker_new(m_engine);
	wasmtime_error_t *error = wasmtime_linker_define_wasi(Linker);
	if (error != NULL)
		exit_with_error("failed to link wasi", error, NULL);

	wasm_byte_vec_t wasm;
	// Load our input file to parse it next
	FILE *file;
	if (fopen_s(&file, inFile.c_str(), "rb") || !file)
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

	wasmtime_module_t *Module = nullptr;
	wasmtime_extern_t Run{};
	wasmtime_extern_t Memory{};

	error = wasmtime_module_new(m_engine, (uint8_t *)wasm.data, wasm.size, &Module);
	if (!Module)
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

	error = wasmtime_context_set_wasi(Context, wasi_config);
	if (error != NULL)
		exit_with_error("failed to instantiate WASI", error, NULL);

	error = wasmtime_linker_module(Linker, Context, "", 0, Module);
	if (error != NULL)
		exit_with_error("failed to instantiate module", error, NULL);

	if (!wasmtime_linker_get(Linker, Context, "", 0, "start", 5, &Run))
		return -1;

	if (!wasmtime_linker_get(Linker, Context, "", 0, "memory", 6, &Memory))
		return -1;

	wasmtime_val_t results[1];

	wasm_trap_t *trap = nullptr;
	error =
		wasmtime_func_call(Context, &Run.of.func, nullptr, 0, results, 1, &trap);

	if (error != NULL || trap != NULL)
		exit_with_error("failed to call run", error, trap);

	auto mem = wasmtime_memory_data(Context, &Memory.of.memory);
	auto offset = results[0].of.i32;

	// CreateBuffer with this
	auto length = *reinterpret_cast<uint32_t *>(mem + offset);
	auto str = reinterpret_cast<char *>(mem + offset + 4);

	auto objmtl = std::string(str, str + length);

	std::string delimiter = "mtllib";
	std::string mtl = objmtl.substr(0, objmtl.find(delimiter));
	std::string obj = objmtl.erase(0, objmtl.find(delimiter));

	/*
	auto rawname = inFile.substr(0, inFile.find_last_of("."));
	std::ofstream out_obj(rawname + ".obj");
	std::ofstream out_mtl(rawname + ".mtl");
	*/

	std::ofstream out_obj("demo.obj");
	std::ofstream out_mtl("demo.obj.mtl");

	out_obj << obj;
	out_obj.close();
	
	out_mtl << mtl;
	out_mtl.close();

	return 0;
}
