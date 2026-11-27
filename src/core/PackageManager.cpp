#include "PackageManager.hpp"
#include <Windows.h>
#include <Shellapi.h>
#include <algorithm>

#pragma comment(lib, "Shell32.lib")

namespace core {
const std::vector<PackageInfo>& GetPackages() {
    static const std::vector<PackageInfo> packages = {
        { L"Brave Browser", L"Brave.Brave", L"Navegador com foco em privacidade" },
        { L"Discord", L"Discord.Discord", L"Comunicação e comunidades" },
        { L"Visual Studio Code", L"Microsoft.VisualStudioCode", L"Editor de código leve" },
        { L"Visual Studio 2022 Community", L"Microsoft.VisualStudio.2022.Community", L"IDE completa para desenvolvimento" },
        { L"Git", L"Git.Git", L"Controle de versão" },
        { L"7-Zip", L"7zip.7zip", L"Compactador de arquivos" },
        { L"PowerToys", L"Microsoft.PowerToys", L"Utilitários extras para Windows" },
        { L"VLC", L"VideoLAN.VLC", L"Reprodutor multimídia" },
        { L"Firefox", L"Mozilla.Firefox", L"Navegador alternativo" },
        { L"Spotify", L"Spotify.Spotify", L"Aplicativo de música" }
    };
    return packages;
}

bool IsWingetAvailable() {
    wchar_t path[MAX_PATH]{};
    return SearchPathW(nullptr, L"winget.exe", nullptr, MAX_PATH, path, nullptr) > 0;
}

bool LaunchPackageInstall(const PackageInfo& package) {
    // Accept only IDs from the built-in catalog. This prevents callers from
    // injecting arbitrary command-line arguments through PackageInfo.
    const auto& catalog = GetPackages();
    const auto trusted = std::find_if(catalog.begin(), catalog.end(),
        [&](const PackageInfo& item) { return item.id == package.id; });
    if (trusted == catalog.end())
        return false;

    if (!IsWingetAvailable()) {
        ShellExecuteW(nullptr, L"open",
            L"ms-windows-store://pdp/?ProductId=9NBLGGH4NNS1",
            nullptr, nullptr, SW_SHOWNORMAL);
        return false;
    }

    // The package ID is now guaranteed to come from the fixed allowlist.
    const std::wstring command = L"/k winget install --id \"" + trusted->id +
        L"\" -e --source winget";
    const auto result = reinterpret_cast<INT_PTR>(
        ShellExecuteW(nullptr, L"open", L"cmd.exe", command.c_str(), nullptr, SW_SHOWNORMAL));
    return result > 32;
}
}
