#include "Cleanup.hpp"
#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <filesystem>
#include <string>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")

namespace core {
namespace {
bool IsSafeTempRoot(const std::wstring& path) {
    if (path.empty() || path.size() < 8) return false;
    const DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
        return false;
    if (attrs & FILE_ATTRIBUTE_REPARSE_POINT) return false;

    PWSTR localAppData = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &localAppData)))
        return false;
    std::wstring expected = localAppData;
    CoTaskMemFree(localAppData);
    expected += L"\\Temp";
    return _wcsicmp(path.c_str(), expected.c_str()) == 0;
}

std::uint64_t MeasureDirectory(const std::wstring& folder) {
    std::uint64_t total = 0;
    WIN32_FIND_DATAW data{};
    const std::wstring pattern = folder + L"\\*";
    HANDLE find = FindFirstFileW(pattern.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) return 0;
    do {
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0)
            continue;
        const std::wstring item = folder + L"\\" + data.cFileName;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            continue;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            total += MeasureDirectory(item);
        else
            total += (static_cast<std::uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    } while (FindNextFileW(find, &data));
    FindClose(find);
    return total;
}

void RemoveContents(const std::wstring& folder, CleanupResult& result) {
    WIN32_FIND_DATAW data{};
    const std::wstring pattern = folder + L"\\*";
    HANDLE find = FindFirstFileW(pattern.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0)
            continue;

        const std::wstring item = folder + L"\\" + data.cFileName;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            ++result.skipped;
            continue;
        }

        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            RemoveContents(item, result);
            if (RemoveDirectoryW(item.c_str()))
                ++result.directoriesRemoved;
            else
                ++result.skipped;
        } else {
            const std::uint64_t size =
                (static_cast<std::uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
            if (DeleteFileW(item.c_str())) {
                result.bytesFreed += size;
                ++result.filesRemoved;
            } else {
                ++result.skipped;
            }
        }
    } while (FindNextFileW(find, &data));
    FindClose(find);
}
}

std::wstring GetTemporaryDirectory() {
    PWSTR localAppData = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &localAppData)))
        return {};
    std::wstring path = localAppData;
    CoTaskMemFree(localAppData);
    path += L"\\Temp";
    return IsSafeTempRoot(path) ? path : std::wstring{};
}

std::uint64_t GetTemporaryFilesSize() {
    const auto path = GetTemporaryDirectory();
    return path.empty() ? 0 : MeasureDirectory(path);
}

CleanupResult CleanTemporaryFiles() {
    CleanupResult result{};
    const auto path = GetTemporaryDirectory();
    if (path.empty()) {
        result.skipped = 1;
        return result;
    }
    RemoveContents(path, result);
    return result;
}

bool EmptyRecycleBin() {
    const int status = SHEmptyRecycleBinW(nullptr, nullptr,
        SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
    return status == S_OK || status == S_FALSE;
}
}
