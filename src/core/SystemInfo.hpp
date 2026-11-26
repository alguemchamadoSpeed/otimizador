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
    std::uint64_t memoryTotalBytes = 0;
    std::uint64_t memoryAvailableBytes = 0;
    std::uint64_t diskFreeBytes = 0;
    std::uint64_t diskTotalBytes = 0;
    unsigned long build = 0;
};

SystemSnapshot GetSystemSnapshot();
}
