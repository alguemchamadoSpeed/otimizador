#include "ui/MainWindow.hpp"
#include <Windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    // Resolve the Windows 10 DPI API dynamically so older Windows 10 builds can still start.
    using SetDpiContextFn = BOOL(WINAPI*)(HANDLE);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    auto setDpiContext = user32
        ? reinterpret_cast<SetDpiContextFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"))
        : nullptr;
    if (setDpiContext)
        setDpiContext(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4))); // Per-monitor v2
    return ui::RunMainWindow(instance, showCommand);
}
