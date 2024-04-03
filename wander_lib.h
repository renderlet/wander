#pragma once

#include "wander.h"

#include <fstream>
#include <iostream>
#include <ostream>
#include <queue>
#include <utility>
#include <memory>

// TODO - this should only be a private dependency
// Required for wasmtime (missing header)
#include <string>
#include "wasmtime.h"

namespace rive
{
namespace pls
{
	class PLSRenderContext;
}
}

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct IDXGISwapChain1;

typedef unsigned int GLuint;

namespace wander
{

class CByteArrayStreambuf final : public std::streambuf
{
public:
	CByteArrayStreambuf(const uint8_t *begin, size_t size);
	~CByteArrayStreambuf() override = default;
	// copying not allowed
	CByteArrayStreambuf(const CByteArrayStreambuf &) = delete;
	CByteArrayStreambuf &operator=(const CByteArrayStreambuf &) = delete;
	CByteArrayStreambuf(CByteArrayStreambuf &&) = delete;
	CByteArrayStreambuf &operator=(CByteArrayStreambuf &&) = delete;

private:
	int_type underflow() override;
	int_type uflow() override;
	int_type pbackfail(int_type ch) override;
	std::streamsize showmanyc() override;
	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way,
						   std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;
	std::streampos seekpos(std::streampos sp,
						   std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

	const uint8_t *const m_begin;
	const uint8_t *const m_end;
	const uint8_t *m_current;
};


class IBinaryStream
{
protected:
	explicit IBinaryStream(std::iostream &stream) : m_stream(stream)
	{
	}

public:
	template <typename T>
	IBinaryStream &operator>>(T &val)
	{
		m_stream.read(reinterpret_cast<char *>(&val), sizeof(val));
		return *this;
	}

	template <typename T>
	T ReadType()
	{
		T t{};
		m_stream.read(reinterpret_cast<char *>(&t), sizeof(t));
		return t;
	}

	template <typename T>
	IBinaryStream &operator<<(const T &val)
	{
		m_stream.write(reinterpret_cast<const char *>(&val), sizeof(val));
		return *this;
	}

	IBinaryStream &Read(void *val, int size)
	{
		m_stream.read(static_cast<char *>(val), size);
		return *this;
	}

	IBinaryStream &Write(const void *val, int size)
	{
		m_stream.write(static_cast<const char *>(val), size);
		return *this;
	}

	[[nodiscard]] std::string ReadString(int count) const
	{
		const auto bytes = std::unique_ptr<char[]>(new char[count]);

		m_stream.read(bytes.get(), count);
		return std::string(bytes.get(), count);
	}

	[[nodiscard]] std::string ReadCString() const
	{
		std::string buffer;
		std::getline(m_stream, buffer, '\0');
		return buffer;
	}

	bool operator!() const
	{
		return !m_stream;
	}

	explicit operator bool() const
	{
		return m_stream.operator bool();
	}

	[[nodiscard]] int GetPosition() const
	{
		// This is a problem with ReSharper - no issue here
		// ReSharper disable once CppRedundantCastExpression
		return static_cast<int>(m_stream.tellg());
	}

	void SetPosition(int pos, std::ios_base::seekdir dir = std::fstream::beg) const
	{
		m_stream.seekg(pos, dir);
		m_stream.seekp(pos, dir);
	}

protected:
	std::iostream &m_stream;
};


class BinaryMemoryStream final : public IBinaryStream
{
public:
	explicit BinaryMemoryStream(uint8_t *pointer, int length) :
		IBinaryStream(m_iostream), m_buf(pointer, length), m_iostream(&m_buf), m_pointer(pointer),
		m_length(length)
	{
	}

	[[nodiscard]] uint8_t *GetPointer() const
	{
		return m_pointer;
	}

	[[nodiscard]] int GetLength() const
	{
		return m_length;
	}

private:
	CByteArrayStreambuf m_buf;
	std::iostream m_iostream;
	uint8_t *m_pointer;
	int m_length;
};

class VectorLoader
{
public:
	enum class EPathType
	{
		Move = 1,
		Line = 2,
		Quad = 3,
		Cubic = 4,
		Close = 5
	};

	struct Point
	{
		float x;
		float y;
	};

	enum class RenderPaintStyle
	{
		stroke,
		fill
	};

	enum class StrokeJoin : unsigned int
	{
		/// Makes a sharp corner at the joint.
		miter = 0,

		/// Smoothens the joint with a circular/semi-circular shape at the
		/// joint.
		round = 1,

		/// Creates a beveled edge at the joint.
		bevel = 2
	};

	enum class StrokeCap : unsigned int
	{
		/// Flat edge at the start/end of the stroke.
		butt = 0,

		/// Circular edge at the start/end of the stroke.
		round = 1,

		/// Flat protruding edge at the start/end of the stroke.
		square = 2
	};

	enum class BlendMode : unsigned char
	{
		srcOver = 3,
		screen = 14,
		overlay = 15,
		darken = 16,
		lighten = 17,
		colorDodge = 18,
		colorBurn = 19,
		hardLight = 20,
		softLight = 21,
		difference = 22,
		exclusion = 23,
		multiply = 24,
		hue = 25,
		saturation = 26,
		color = 27,
		luminosity = 28
	};

	struct RenderPaint
	{
		RenderPaintStyle style;
		uint32_t color;
		float thickness;
		StrokeJoin join;
		StrokeCap cap;
		BlendMode blendMode;
	};

	typedef std::tuple<EPathType, std::vector<Point>> Path;
	typedef std::vector<Path> PathList;
	typedef std::tuple<RenderPaint, PathList> Command;

	static auto Read(uint8_t *p_data, uint32_t length) -> std::vector<Command>
	{
		BinaryMemoryStream ms(p_data, length);

		std::vector<Command> command_list;

		while (ms)
		{
			RenderPaint p{};
			ms >> p;

			int path_size = 0;
			ms >> path_size;

			if (path_size == 0)
				break;

			PathList path_list(path_size);

			for (auto& path : path_list)
			{
				EPathType type;
				ms >> type;

				int points_size = 0;
				ms >> points_size;

				std::vector<Point> points(points_size);

				for (auto& point: points)
				{
					ms >> point.x;
					ms >> point.y;
				}

				std::get<0>(path) = type;
				std::get<1>(path) = points;
			}

			command_list.emplace_back(p, path_list);
		}
		return command_list;
	}

	std::vector<Command> m_command_list;
};

class Pal : public IPal
{
public:  // TODO: Replace with std::span
	virtual ObjectID CreateBuffer(BufferDescriptor desc, int length, uint8_t data[]) = 0;
	virtual ObjectID CreateTexture(BufferDescriptor desc, int length, uint8_t data[]) = 0;
	virtual void DeleteBuffer(ObjectID buffer_id) = 0;

	virtual ObjectID CreateVector(int length, uint8_t data[]) = 0;
	virtual ObjectID UpdateVector(int length, uint8_t data[], ObjectID buffer_id) = 0;

	virtual void DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned int stride) = 0;

	virtual void DrawVector(ObjectID buffer_id, int slot, int width, int height) = 0;
};


#ifdef _WIN32

class PalD3D11 : public Pal
{
public:
	PalD3D11(ID3D11Device* device, ID3D11DeviceContext* context);
	PalD3D11(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain1* swapchain);

	virtual ~PalD3D11();

	void Release() override{};

	EPalType Type() override
	{
		return EPalType::D3D11;
	}

	ObjectID CreateBuffer(BufferDescriptor desc, int length, uint8_t data[]) override;
	ObjectID CreateTexture(BufferDescriptor desc, int length, uint8_t data[]) override;
	void DeleteBuffer(ObjectID buffer_id) override;

	ObjectID CreateVector(int length, uint8_t data[]) override;
	ObjectID UpdateVector(int length, uint8_t data[], ObjectID buffer_id) override;

	void DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned int stride) override;

	void DrawVector(ObjectID buffer_id, int slot, int width, int height) override;
	

private:
	std::vector<ID3D11Buffer*> m_buffers;
	std::vector<ID3D11Texture2D*> m_textures;

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_device_context;
	IDXGISwapChain1 *m_swapchain;

#if defined(RLT_RIVE) && defined(_WIN32)
	std::unique_ptr<rive::pls::PLSRenderContext> m_plsContext;
	std::vector<std::vector<VectorLoader::Command>> m_vector_commands;
	std::vector<ID3D11Texture2D *> m_vector_textures;
	std::vector<ID3D11ShaderResourceView*> m_vector_srvs;
#endif
};

#endif


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

	ObjectID CreateVector(int length, uint8_t data[]) override;
	ObjectID UpdateVector(int length, uint8_t data[], ObjectID buffer_id) override;

	void DrawTriangleList(ObjectID buffer_id, int offset, int length, unsigned int stride) override;

	void DrawVector(ObjectID buffer_id, int slot, int width, int height) override;

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

	ObjectID Render(ObjectID renderlet_id, ObjectID tree_id = -1) override;

	const RenderTree *GetRenderTree(ObjectID tree_id) override;
	void DestroyRenderTree(ObjectID tree_id) override;

	void Release() override;

	Pal* PalImpl() const
	{
		return m_pal;
	}

private:

	ObjectID BuildVector(uint32_t length, uint8_t *data, ObjectID tree_id);

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
