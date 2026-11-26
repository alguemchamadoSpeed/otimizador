#pragma once
#include <string>
#include <vector>

namespace core {
struct PackageInfo {
    std::wstring name;
    std::wstring id;
    std::wstring description;
};

const std::vector<PackageInfo>& GetPackages();
bool IsWingetAvailable();
bool LaunchPackageInstall(const PackageInfo& package);
}
