#include "SystemInfo.hpp"
#include <Windows.h>
#include <winternl.h>
#include <winreg.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "Advapi32.lib")

namespace core {
namespace {
using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);

std::wstring ReadProcessorName() {
    HKEY key = nullptr;
    wchar_t value[512]{};
    DWORD type = 0;
    DWORD bytes = sizeof(value);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
        RegQueryValueExW(key, L"ProcessorNameString", nullptr, &type,
            reinterpret_cast<BYTE*>(value), &bytes);
        RegCloseKey(key);
    }
    std::wstring result = value;
    while (!result.empty() && (result.back() == L' ' || result.back() == L'\0'))
        result.pop_back();
    return result.empty() ? L"Não detectado" : result;
}

std::wstring FormatBytes(ULONGLONG bytes) {
    static const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) { value /= 1024.0; ++unit; }
    std::wostringstream out;
    out << std::fixed << std::setprecision(unit == 0 ? 0 : 1) << value << L" " << units[unit];
    return out.str();
}
}

SystemSnapshot GetSystemSnapshot() {
    SystemSnapshot info{};
    OSVERSIONINFOEXW version{};
    version.dwOSVersionInfoSize = sizeof(version);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    auto rtlGetVersion = ntdll
        ? reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"))
        : nullptr;
    if (rtlGetVersion && rtlGetVersion(reinterpret_cast<PRTL_OSVERSIONINFOW>(&version)) == 0) {
        info.build = version.dwBuildNumber;
        info.windowsName = version.dwBuildNumber >= 22000 ? L"Windows 11" : L"Windows 10";
        std::wostringstream v;
        v << version.dwMajorVersion << L"." << version.dwMinorVersion
          << L"  •  Build " << version.dwBuildNumber;
        info.windowsVersion = v.str();
    } else {
        info.windowsName = L"Windows";
        info.windowsVersion = L"Versão não disponível";
    }

    info.processor = ReadProcessorName();

    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) {
        info.memoryTotalBytes = memory.ullTotalPhys;
        info.memoryAvailableBytes = memory.ullAvailPhys;
        info.memory = FormatBytes(memory.ullTotalPhys);
        info.memoryDetail = FormatBytes(memory.ullAvailPhys) + L" disponíveis";
    } else {
        info.memory = L"Não detectada";
    }

    wchar_t windowsDir[MAX_PATH]{};
    if (GetWindowsDirectoryW(windowsDir, MAX_PATH)) {
        wchar_t root[] = L"C:\\";
        root[0] = windowsDir[0];
        ULARGE_INTEGER freeBytes{}, totalBytes{}, totalFree{};
        if (GetDiskFreeSpaceExW(root, &freeBytes, &totalBytes, &totalFree)) {
            info.diskFreeBytes = freeBytes.QuadPart;
            info.diskTotalBytes = totalBytes.QuadPart;
            info.diskFree = FormatBytes(freeBytes.QuadPart);
            info.diskDetail = L"livres de " + FormatBytes(totalBytes.QuadPart);
        }
    }
    if (info.diskFree.empty()) {
        info.diskFree = L"Indisponível";
        info.diskDetail.clear();
    }
    return info;
}
}
