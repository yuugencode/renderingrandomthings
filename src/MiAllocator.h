#pragma once

#include <mimalloc.h>
#include <bx/allocator.h>
#include <cassert>

// Bunch of not very exciting boilerplate

// Replace CRT malloc with mimalloc
#pragma warning(disable: 4595)
inline void* operator new(size_t size) { return mi_malloc(size); }
inline void operator delete(void* ptr) { mi_free(ptr); }
#pragma warning(default: 4595)

// malloc/free (and stl containers?) are beyond saving, needs project-wide overriding with mimalloc_redirect

// bgfx needs extra effort
struct MiAllocator : bx::AllocatorI {

	virtual void* realloc(void* _ptr, size_t _size, size_t _align, const char* _filePath, uint32_t _line) {

		// As per bgfx definition:
		///  - Allocate memory block: _ptr == NULL && size > 0
		///  -   Resize memory block: _ptr != NULL && size > 0
		///  -     Free memory block: _ptr != NULL && size == 0

		if (_ptr == nullptr && _size == 0) return nullptr; // bgfx likes to call this with null, 0 params...

		if (_align == 0) { // Malloc
			if (_ptr == nullptr) return mi_malloc(_size);
			if (_size > size_t(0)) return mi_realloc(_ptr, _size);
			if (_size == 0) mi_free(_ptr);
		}
		else { // Aligned alloc
			if (_ptr == nullptr) return mi_aligned_alloc(_align, _size);
			if (_size > size_t(0)) return mi_realloc_aligned(_ptr, _size, _align);
			if (_size == 0) mi_free_aligned(_ptr, _align);
		}
		return nullptr;
	}
};
