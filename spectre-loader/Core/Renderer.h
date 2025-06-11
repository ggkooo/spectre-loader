#pragma once
#include <d3d11.h>
#include <dxgi.h>

class Renderer {
public:
    static bool Initialize(HWND hwnd);
    static void Shutdown();
    static void BeginFrame();
    static void EndFrame();

    static ID3D11Device* GetDevice() { return s_device; }
    static ID3D11DeviceContext* GetContext() { return s_deviceContext; }

private:
    static ID3D11Device* s_device;
    static ID3D11DeviceContext* s_deviceContext;
    static IDXGISwapChain* s_swapChain;
    static ID3D11RenderTargetView* s_renderTargetView;

    static bool CreateDeviceD3D(HWND hwnd);
    static void CleanupDeviceD3D();
    static void CreateRenderTarget();
    static void CleanupRenderTarget();
};
