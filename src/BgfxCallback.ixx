module;

#include <bgfx/bgfx.h>

export module BgfxCallback;

import Log;

/// <summary> This struct is needed to get debug info from bgfx.. </summary>
export struct BgfxCallback : public bgfx::CallbackI {

	virtual void fatal(const char* _filePath, uint16_t _line, bgfx::Fatal::Enum _code, const char* _str) override {
		// Something unexpected happened, inform user and bail out.
		// Must terminate, continuing will cause crash anyway.
		Log::Formatted("Fatal error: {}, {}", (int)_code, _str);
		abort();
	}

	virtual void debugPrintfVargs(const char* _format, va_list _argList) {
		char temp[8192];
		char* out = temp;
		int32_t len = vsnprintf(out, sizeof(temp), _format, _argList);
		if ((int32_t)sizeof(temp) < len)
		{
			out = (char*)_alloca(len + 1);
			len = vsnprintf(out, len, _format, _argList);
		}
		out[len] = '\0';
		printf(out);
	}

	virtual void traceVargs(const char* _filePath, uint16_t _line, const char* _format, va_list _argList) override {
		debugPrintfVargs(_format, _argList);
	}

	virtual void profilerBegin(const char* /*_name*/, uint32_t /*_abgr*/, const char* /*_filePath*/, uint16_t /*_line*/) override { }
	virtual void profilerBeginLiteral(const char* /*_name*/, uint32_t /*_abgr*/, const char* /*_filePath*/, uint16_t /*_line*/) override { }
	virtual void profilerEnd() override { }
	virtual uint32_t cacheReadSize(uint64_t _id) override { return 0; }
	virtual bool cacheRead(uint64_t _id, void* _data, uint32_t _size) override { return false; }
	virtual void cacheWrite(uint64_t _id, const void* _data, uint32_t _size) override { }
	virtual void screenShot(const char* _filePath, uint32_t _width, uint32_t _height, uint32_t _pitch, const void* _data, uint32_t /*_size*/, bool _yflip) override { }
	virtual void captureBegin(uint32_t _width, uint32_t _height, uint32_t /*_pitch*/, bgfx::TextureFormat::Enum /*_format*/, bool _yflip) override { }
	virtual void captureEnd() override { }
	virtual void captureFrame(const void* _data, uint32_t /*_size*/) override { }
	virtual ~BgfxCallback() override { }
};

// C++20 Modules + Intellisense bug: 
// When calling BgfxCallback constructor outside the module it's defined in,
// intellisense goes completely wild and starts seeing errors even if the code compiles just fine
// Hacky workaround: Create the object in the module itself and manually delete later
export BgfxCallback* CreateBgfxCallback() {
#if _DEBUG
	return new BgfxCallback();
#else
	return nullptr;
#endif
}
