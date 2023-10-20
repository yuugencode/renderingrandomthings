module;

#include <sdl/SDL.h>
#include <bgfx/bgfx.h>
#include "imgui/imgui.h"
#include <glm/glm.hpp>

export module ImguiDrawer;

import <vector>;
import <filesystem>;
import <fstream>;
import Window;
import Log;
import Input;

/// <summary> Contains helper functions to work with imgui functions </summary>
export class ImguiDrawer {

    inline static bool initialized = false;
	inline static bgfx::VertexLayout  vertexLayout;
	inline static bgfx::TextureHandle fontTexture;
	inline static bgfx::UniformHandle fontUniform;
	inline static bgfx::ProgramHandle program;

    static const bgfx::Memory* LoadMemory(const std::filesystem::path& path) {
		if (!std::filesystem::exists(path)) return nullptr;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        const bgfx::Memory* mem = bgfx::alloc(uint32_t(size + 1)); // Bgfx deals with freeing; leaks on read fail?
        if (file.read((char*)mem->data, size)) {
            mem->data[mem->size - 1] = '\0';
            return mem;
        }
        return nullptr;
    }

    static bgfx::ShaderHandle LoadShader(const std::filesystem::path& path) {
		auto buffer = LoadMemory(path);
		assert(buffer != nullptr);
        return bgfx::createShader(buffer);
    }

    ImguiDrawer() {}
    ~ImguiDrawer() {}

public:

	static inline const bgfx::ViewId IMGUI_VIEW_IDX = 1;

    static void Init(const Window& window) {
        if (initialized) return;

		IMGUI_CHECKVERSION();
        ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

        // Vertex layout
        vertexLayout.begin();
        vertexLayout.add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float);
        vertexLayout.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
        vertexLayout.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true);
        vertexLayout.end();

		// Create font
		unsigned char* data;
		int width, height;
		io.Fonts->AddFontDefault();
		io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);
		fontTexture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::BGRA8, 0, bgfx::copy(data, width * height * 4));
		fontUniform = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

		// shadercRelease.exe -f imgui_vs.sh -o imgui_vs_comp.bin -i "." --type v --platform windows --profile s_5_0
		// shadercRelease.exe -f imgui_fs.sh -o imgui_fs_comp.bin -i "." --type f --platform windows --profile s_5_0

		// Read and compile shaders
		const std::filesystem::path folder = "shaders";
        auto vShader = LoadShader(folder / "imgui_vs_comp.bin");
        auto fShader = LoadShader(folder / "imgui_fs_comp.bin");

		assert(vShader.idx != bgfx::kInvalidHandle);
		assert(fShader.idx != bgfx::kInvalidHandle);

        program = bgfx::createProgram(vShader, fShader, true);
		assert(program.idx != bgfx::kInvalidHandle);

		bgfx::setViewRect(IMGUI_VIEW_IDX, 0, 0, bgfx::BackbufferRatio::Equal);
    }

	static void NewFrame(const Window& window, float deltaTime) {
		
		ImGuiIO& io = ImGui::GetIO();

		io.DisplaySize = ImVec2((float)window.width, (float)window.height);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
		io.DeltaTime = deltaTime;

		int x, y;
		Uint32 buttons = SDL_GetMouseState(&x, &y);
		io.MousePos = ImVec2((float)x, (float)y);
		io.AddMouseButtonEvent(ImGuiMouseButton_Left, Input::MouseHeld(SDL_BUTTON_LEFT, true));
		io.AddMouseButtonEvent(ImGuiMouseButton_Middle, Input::MouseHeld(SDL_BUTTON_MIDDLE, true));
		io.AddMouseButtonEvent(ImGuiMouseButton_Right, Input::MouseHeld(SDL_BUTTON_RIGHT, true));

		glm::vec2 wheel = Input::GetMouseWheel();
		io.AddMouseWheelEvent(wheel.x, wheel.y);

		if (io.WantCaptureMouse) {
			Input::DevalidateMouse(SDL_BUTTON_LEFT);
			Input::DevalidateMouse(SDL_BUTTON_MIDDLE);
			Input::DevalidateMouse(SDL_BUTTON_RIGHT);
		}

		bgfx::touch(IMGUI_VIEW_IDX);

		ImGui::NewFrame();
	}

	static void TestDraws() {
		ImGui::ShowDemoWindow();
	}

	/// Loosely based on now seemingly unmaintained bigg-library's public domain implementation
	static void Render() {

		ImGui::Render();

        auto drawData = ImGui::GetDrawData();

		if (drawData == nullptr) return; // Something off

		for (int ii = 0, num = drawData->CmdListsCount; ii < num; ++ii) {
			bgfx::TransientVertexBuffer tvb;
			bgfx::TransientIndexBuffer tib;

			const ImDrawList* drawList = drawData->CmdLists[ii];
			uint32_t numVertices = (uint32_t)drawList->VtxBuffer.size();
			uint32_t numIndices = (uint32_t)drawList->IdxBuffer.size();

			if (!bgfx::getAvailTransientVertexBuffer(numVertices, vertexLayout) || !bgfx::getAvailTransientIndexBuffer(numIndices))
				break;

			bgfx::allocTransientVertexBuffer(&tvb, numVertices, vertexLayout);
			bgfx::allocTransientIndexBuffer(&tib, numIndices);

			ImDrawVert* verts = (ImDrawVert*)tvb.data;
			memcpy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert));

			ImDrawIdx* indices = (ImDrawIdx*)tib.data;
			memcpy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx));

			uint32_t offset = 0;
			for (const ImDrawCmd* cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd) {
				if (cmd->UserCallback) {
					cmd->UserCallback(drawList, cmd);
				}
				else if (0 != cmd->ElemCount) {
					uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;
					bgfx::TextureHandle th = fontTexture;
					if (cmd->TextureId != NULL)
						th.idx = uint16_t(uintptr_t(cmd->TextureId));

					state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
					const uint16_t xx = uint16_t(glm::max(cmd->ClipRect.x, 0.0f));
					const uint16_t yy = uint16_t(glm::max(cmd->ClipRect.y, 0.0f));
					bgfx::setScissor(xx, yy, uint16_t(glm::min(cmd->ClipRect.z, 65535.0f) - xx), uint16_t(glm::min(cmd->ClipRect.w, 65535.0f) - yy));
					bgfx::setState(state);
					bgfx::setTexture(0, fontUniform, th);
					bgfx::setVertexBuffer(0, &tvb, 0, numVertices);
					bgfx::setIndexBuffer(&tib, offset, cmd->ElemCount);
					bgfx::submit(IMGUI_VIEW_IDX, program);
				}

				offset += cmd->ElemCount;
			}
		}
    }

	static void Quit() {
		bgfx::destroy(fontUniform);
		bgfx::destroy(fontTexture);
		bgfx::destroy(program);
		ImGui::DestroyContext();
	}
};