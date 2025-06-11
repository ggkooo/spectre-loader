#pragma once
#include <windows.h>
#include <dwmapi.h>

class Window {
public:
    static bool Initialize(HINSTANCE hInstance, int width, int height);
    static void Shutdown();
    static HWND GetHandle() { return s_hwnd; }
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    static HWND s_hwnd;
    static void RemoveWindowBorder();
};
