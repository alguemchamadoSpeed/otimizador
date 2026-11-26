#pragma once
#include <cstdint>
#include <string>

namespace core {
struct SystemSnapshot {
    std::wstring windowsName;
    std::wstring windowsVersion;
    std::wstring processor;
    std::wstring memory;
    std::wstring memoryDetail;
    std::wstring diskFree;
    std::wstring diskDetail;
    unsigned long build = 0;
};

SystemSnapshot GetSystemSnapshot();
}
