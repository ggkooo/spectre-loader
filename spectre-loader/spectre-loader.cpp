// spectre-loader.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <d3d11.h>
#include <dxgi.h>
#include "imgui/imgui/imgui.h"
#include "imgui/imgui/imgui_impl_win32.h"
#include "imgui/imgui/imgui_impl_dx11.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "assets/images/icons.h"

// DirectX 11 objects
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
ID3D11ShaderResourceView* g_CloseButtonTexture = nullptr;
ID3D11ShaderResourceView* g_MinimizeButtonTexture = nullptr;


// Forward declarations
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

ID3D11ShaderResourceView* LoadTextureFromMemory(const unsigned char* image_data, size_t image_size) {
    // Load from memory using stb_image
    int width, height, channels;
    unsigned char* data = stbi_load_from_memory(image_data, image_size, &width, &height, &channels, 4);
    if (!data) return nullptr;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;

    ID3D11Texture2D* texture = nullptr;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &texture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;

    ID3D11ShaderResourceView* srv = nullptr;
    g_pd3dDevice->CreateShaderResourceView(texture, &srvDesc, &srv);
    texture->Release();

    stbi_image_free(data);
    return srv;
}


int main() {
    // Create application window
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"ImGui Window", nullptr };
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowW(wc.lpszClassName, L"ImGui Window",
        WS_POPUP,
        (GetSystemMetrics(SM_CXSCREEN) - 700) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - 400) / 2,
        700, 400,
        nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Set the ImGui window to be full size with no title bar and non-movable
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(700, 400));

        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoMove |             // Prevent window movement
            ImGuiWindowFlags_NoResize |           // Prevent window resizing
            ImGuiWindowFlags_NoTitleBar |         // Remove title bar
            ImGuiWindowFlags_NoCollapse |         // Remove collapse button
            ImGuiWindowFlags_NoScrollbar |        // Remove scrollbars
            ImGuiWindowFlags_NoBringToFrontOnFocus; // Prevent window from being brought to front

        if (g_pd3dDevice)
        {
            // Load the button textures
            g_CloseButtonTexture = LoadTextureFromMemory(CLOSE_ICON, CLOSE_ICON_SIZE);
            g_MinimizeButtonTexture = LoadTextureFromMemory(MINIMIZE_ICON, MINIMIZE_ICON_SIZE);
        }

        ImGui::Begin("##MainWindow", nullptr, window_flags);        

        // Add control buttons in top-right corner
        float windowWidth = ImGui::GetWindowWidth();
        float rightPadding = 20.0f;  // Padding from right edge
        float buttonWidth = 15.0f;   // Width of each button
        float buttonSpacing = 10.0f; // Space between buttons
        float topPadding = 5.0f;    // Distance from top edge

        // Style for the buttons (transparent background)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        // Minimize button
        ImGui::SetCursorPos(ImVec2(windowWidth - (buttonWidth * 2 + buttonSpacing + rightPadding), topPadding));
        if (ImGui::ImageButton("##minimize", g_MinimizeButtonTexture,
            ImVec2(buttonWidth, 15), ImVec2(0, 0), ImVec2(1, 1),
            ImVec4(0, 0, 0, 0),          // bg color
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f)))
        {
            ShowWindow(GetActiveWindow(), SW_MINIMIZE);
        }

        // Close button
        ImGui::SameLine();
        ImGui::SetCursorPosX(windowWidth - (buttonWidth + rightPadding));
        if (ImGui::ImageButton("##close", g_CloseButtonTexture,
            ImVec2(buttonWidth, 15), ImVec2(0, 0), ImVec2(1, 1),
            ImVec4(0, 0, 0, 0),          // bg color
            ImVec4(1.0f, 1.0f, 1.0f, 0.75f)))
        {
            PostQuitMessage(0);
        }

        ImGui::PopStyleColor(3);

        // Rest of your window content
        ImGui::Text("Hello, world!");
        ImGui::End();

        // Add to your cleanup code before shutting down
        if (g_CloseButtonTexture) { g_CloseButtonTexture->Release(); g_CloseButtonTexture = nullptr; }
        if (g_MinimizeButtonTexture) {
            if (g_MinimizeButtonTexture) { g_MinimizeButtonTexture->Release(); g_MinimizeButtonTexture = nullptr; }
		}

		// Rendering the ImGui frame
        ImGui::Render();
        const ImVec4 clear_color(0.45f, 0.55f, 0.60f, 1.00f);  // Create a named variable
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam),
                DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
