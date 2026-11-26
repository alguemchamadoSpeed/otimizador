#pragma once
#include <cstdint>
#include <string>

namespace core {
struct CleanupResult {
    std::uint64_t bytesFreed = 0;
    unsigned int filesRemoved = 0;
    unsigned int directoriesRemoved = 0;
    unsigned int skipped = 0;
};

std::wstring GetTemporaryDirectory();
std::uint64_t GetTemporaryFilesSize();
CleanupResult CleanTemporaryFiles();
bool EmptyRecycleBin();
}
