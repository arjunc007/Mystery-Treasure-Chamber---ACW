#pragma once
#include <d3d11.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

class ImGuiManager
{
public:
    ImGuiManager() = default;
    ~ImGuiManager();

    bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);

    void BeginFrame();

    void Render();

    void Shutdown();
};