#include "MainWindow.hpp"
#include "../core/SystemInfo.hpp"
#include "../core/Cleanup.hpp"
#include "../core/PackageManager.hpp"
#include "../core/Debloat.hpp"

#include <Windows.h>
#include <ShlObj.h>
#include <Shellapi.h>
#include <WebView2.h>
#include <wrl.h>
#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")

namespace ui {
namespace {
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

constexpr wchar_t WindowClassName[] = L"OtimizadorWebViewWindow";
constexpr UINT WM_APP_WEB_MESSAGE = WM_APP + 11;

HWND g_window = nullptr;
ComPtr<ICoreWebView2Environment> g_environment;
ComPtr<ICoreWebView2Controller> g_controller;
ComPtr<ICoreWebView2> g_webView;
std::atomic_bool g_actionBusy{ false };

struct PostedMessage {
    std::wstring json;
    bool releaseBusy = false;
};

struct ActionOutcome {
    bool success = false;
    std::wstring message;
    bool hasTempBytes = false;
    std::uint64_t tempBytes = 0;
};

std::wstring JsonEscape(const std::wstring& value) {
    std::wostringstream out;
    for (const wchar_t ch : value) {
        switch (ch) {
        case L'"': out << L"\\\""; break;
        case L'\\': out << L"\\\\"; break;
        case L'\b': out << L"\\b"; break;
        case L'\f': out << L"\\f"; break;
        case L'\n': out << L"\\n"; break;
        case L'\r': out << L"\\r"; break;
        case L'\t': out << L"\\t"; break;
        default:
            if (ch < 0x20) {
                constexpr wchar_t hex[] = L"0123456789abcdef";
                out << L"\\u00" << hex[(ch >> 4) & 0xF] << hex[ch & 0xF];
            } else {
                out << ch;
            }
        }
    }
    return out.str();
}

void PostJson(const std::wstring& json) {
    if (g_webView)
        g_webView->PostWebMessageAsJson(json.c_str());
}

std::wstring SnapshotJson() {
    const auto info = core::GetSystemSnapshot();
    std::wostringstream json;
    json << L"{\"type\":\"snapshot\""
         << L",\"windowsName\":\"" << JsonEscape(info.windowsName) << L"\""
         << L",\"windowsVersion\":\"" << JsonEscape(info.windowsVersion) << L"\""
         << L",\"build\":" << info.build
         << L",\"processor\":\"" << JsonEscape(info.processor) << L"\""
         << L",\"memoryTotal\":" << info.memoryTotalBytes
         << L",\"memoryAvailable\":" << info.memoryAvailableBytes
         << L",\"diskFree\":" << info.diskFreeBytes
         << L",\"diskTotal\":" << info.diskTotalBytes
         << L"}";
    return json.str();
}

std::wstring AppsJson() {
    std::wostringstream json;
    json << L"{\"type\":\"apps\",\"items\":[";
    const auto& apps = core::GetOptionalApps();
    for (std::size_t i = 0; i < apps.size(); ++i) {
        if (i) json << L",";
        json << L"{\"id\":\"" << JsonEscape(apps[i].id)
             << L"\",\"name\":\"" << JsonEscape(apps[i].name)
             << L"\",\"description\":\"" << JsonEscape(apps[i].description) << L"\"}";
    }
    json << L"]}";
    return json.str();
}

std::wstring PackagesJson() {
    std::wostringstream json;
    json << L"{\"type\":\"packages\",\"items\":[";
    const auto& packages = core::GetPackages();
    for (std::size_t i = 0; i < packages.size(); ++i) {
        if (i) json << L",";
        json << L"{\"id\":\"" << JsonEscape(packages[i].id)
             << L"\",\"name\":\"" << JsonEscape(packages[i].name)
             << L"\",\"description\":\"" << JsonEscape(packages[i].description) << L"\"}";
    }
    json << L"]}";
    return json.str();
}

std::wstring ActionResultJson(const ActionOutcome& outcome) {
    std::wostringstream json;
    json << L"{\"type\":\"actionResult\",\"success\":"
         << (outcome.success ? L"true" : L"false")
         << L",\"message\":\"" << JsonEscape(outcome.message) << L"\""
         << L",\"refreshSnapshot\":true";
    if (outcome.hasTempBytes)
        json << L",\"tempBytes\":" << outcome.tempBytes;
    json << L"}";
    return json.str();
}

void PostToWindow(HWND hwnd, std::wstring json, bool releaseBusy) {
    auto* message = new (std::nothrow) PostedMessage{ std::move(json), releaseBusy };
    if (!message) {
        if (releaseBusy) g_actionBusy.store(false);
        return;
    }
    if (!PostMessageW(hwnd, WM_APP_WEB_MESSAGE, 0, reinterpret_cast<LPARAM>(message))) {
        delete message;
        if (releaseBusy) g_actionBusy.store(false);
    }
}

void StartTempScan(HWND hwnd) {
    std::thread([hwnd]() {
        const auto bytes = core::GetTemporaryFilesSize();
        std::wostringstream json;
        json << L"{\"type\":\"tempInfo\",\"bytes\":" << bytes << L"}";
        PostToWindow(hwnd, json.str(), false);
    }).detach();
}

ActionOutcome RunAction(const std::wstring& command) {
    ActionOutcome outcome{};

    if (command == L"cleanTemp") {
        const auto result = core::CleanTemporaryFiles();
        outcome.success = result.filesRemoved > 0 || result.directoriesRemoved > 0 || result.skipped == 0;
        std::wostringstream message;
        message << L"Limpeza finalizada. " << result.filesRemoved << L" arquivo(s) removido(s), "
                << result.directoriesRemoved << L" pasta(s) e "
                << result.skipped << L" item(ns) ignorado(s). Espaço liberado: ";
        const double mb = static_cast<double>(result.bytesFreed) / (1024.0 * 1024.0);
        message.setf(std::ios::fixed);
        message.precision(1);
        message << mb << L" MB.";
        outcome.message = message.str();
        outcome.hasTempBytes = true;
        outcome.tempBytes = core::GetTemporaryFilesSize();
        return outcome;
    }

    if (command == L"emptyRecycleBin") {
        outcome.success = core::EmptyRecycleBin();
        outcome.message = outcome.success
            ? L"A Lixeira foi esvaziada."
            : L"Não foi possível esvaziar a Lixeira.";
        return outcome;
    }

    if (command.rfind(L"install:", 0) == 0) {
        const std::wstring id = command.substr(8);
        const auto& packages = core::GetPackages();
        for (const auto& package : packages) {
            if (package.id == id) {
                if (!core::IsWingetAvailable()) {
                    outcome.message = L"winget não está disponível. Atualize o App Installer pela Microsoft Store.";
                    return outcome;
                }
                outcome.success = core::LaunchPackageInstall(package);
                outcome.message = outcome.success
                    ? L"O instalador de " + package.name + L" foi iniciado em uma janela de terminal."
                    : L"Não foi possível iniciar o instalador.";
                return outcome;
            }
        }
        outcome.message = L"Aplicativo não encontrado no catálogo permitido.";
        return outcome;
    }

    if (command.rfind(L"removeApps:", 0) == 0) {
        const std::wstring ids = command.substr(11);
        std::wstringstream list(ids);
        std::wstring id;
        unsigned int successCount = 0;
        unsigned int totalCount = 0;
        std::wostringstream details;
        while (std::getline(list, id, L'|')) {
            if (id.empty()) continue;
            ++totalCount;
            std::wstring resultMessage;
            if (core::RemoveCurrentUserApp(id, resultMessage))
                ++successCount;
            details << resultMessage << L" ";
        }
        outcome.success = totalCount > 0 && successCount == totalCount;
        std::wostringstream message;
        message << successCount << L" de " << totalCount
                << L" aplicativo(s) removido(s). " << details.str();
        outcome.message = message.str();
        return outcome;
    }

    outcome.message = L"Ação não reconhecida.";
    return outcome;
}

void StartAction(HWND hwnd, const std::wstring& command) {
    bool expected = false;
    if (!g_actionBusy.compare_exchange_strong(expected, true)) {
        PostJson(L"{\"type\":\"actionResult\",\"success\":false,\"message\":\"Já existe uma ação em andamento.\"}");
        return;
    }

    std::thread([hwnd, command]() {
        const auto outcome = RunAction(command);
        PostToWindow(hwnd, ActionResultJson(outcome), true);
    }).detach();
}

std::wstring GetExecutableDirectory() {
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    std::wstring result(path, length);
    const auto separator = result.find_last_of(L"\\/");
    if (separator == std::wstring::npos) return {};
    result.resize(separator);
    return result;
}

std::wstring GetWebViewUserDataFolder() {
    PWSTR localAppData = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &localAppData)))
        return {};
    std::wstring path(localAppData);
    CoTaskMemFree(localAppData);
    path += L"\\Otimizador\\WebView2";
    std::error_code error;
    std::filesystem::create_directories(path, error);
    return error ? std::wstring{} : path;
}

void OpenSystemUrl(const std::wstring& target) {
    const wchar_t* url = nullptr;
    if (target == L"startup") url = L"ms-settings:startupapps";
    else if (target == L"apps") url = L"ms-settings:appsfeatures";
    else if (target == L"security") url = L"windowsdefender:";
    else if (target == L"update") url = L"ms-settings:windowsupdate";
    else if (target == L"spotifyLite") url = L"https://github.com/TeamAntiLag/SpotifyLite/releases/tag/SpotifyLite";
    else if (target == L"discordDebloat") url = L"https://github.com/TeamAntiLag/DiscordDebloat/releases/tag/v2.1";
    else if (target == L"winget") url = L"https://learn.microsoft.com/windows/package-manager/winget/";
    if (url) ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

void HandleBridgeMessage(HWND hwnd, const std::wstring& message) {
    if (message == L"getSnapshot" || message == L"refresh") {
        PostJson(SnapshotJson());
        return;
    }
    if (message == L"getApps") {
        PostJson(AppsJson());
        return;
    }
    if (message == L"getPackages") {
        PostJson(PackagesJson());
        return;
    }
    if (message == L"getTempInfo") {
        StartTempScan(hwnd);
        return;
    }
    if (message.rfind(L"open:", 0) == 0) {
        OpenSystemUrl(message.substr(5));
        return;
    }
    if (message == L"cleanTemp" || message == L"emptyRecycleBin" ||
        message.rfind(L"install:", 0) == 0 || message.rfind(L"removeApps:", 0) == 0) {
        StartAction(hwnd, message);
        return;
    }
    PostJson(L"{\"type\":\"actionResult\",\"success\":false,\"message\":\"Comando desconhecido.\"}");
}

void ResizeWebView(HWND hwnd) {
    if (!g_controller) return;
    RECT bounds{};
    GetClientRect(hwnd, &bounds);
    g_controller->put_Bounds(bounds);
}

void ShowWebViewError(HRESULT result) {
    wchar_t message[512]{};
    swprintf_s(message,
        L"Não foi possível iniciar a interface WebView2.\n\n"
        L"Código: 0x%08X\n\n"
        L"Verifique se o Microsoft Edge WebView2 Runtime está instalado. "
        L"Você pode instalá-lo pelo site oficial da Microsoft.",
        static_cast<unsigned int>(result));
    MessageBoxW(g_window, message, L"Otimizador - WebView2", MB_OK | MB_ICONERROR);
}

void InitializeWebView(HWND hwnd) {
    const std::wstring userData = GetWebViewUserDataFolder();
    if (userData.empty()) {
        ShowWebViewError(E_ACCESSDENIED);
        return;
    }

    const HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, userData.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                if (FAILED(result) || !environment) {
                    ShowWebViewError(result);
                    return result;
                }
                g_environment = environment;
                return g_environment->CreateCoreWebView2Controller(
                    hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hwnd](HRESULT controllerResult, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(controllerResult) || !controller) {
                                ShowWebViewError(controllerResult);
                                return controllerResult;
                            }

                            g_controller = controller;
                            HRESULT result = g_controller->get_CoreWebView2(&g_webView);
                            if (FAILED(result) || !g_webView) {
                                ShowWebViewError(result);
                                return result;
                            }

                            ComPtr<ICoreWebView2Settings> settings;
                            if (SUCCEEDED(g_webView->get_Settings(&settings)) && settings) {
#ifdef _DEBUG
                                settings->put_AreDevToolsEnabled(TRUE);
#else
                                settings->put_AreDevToolsEnabled(FALSE);
#endif
                                settings->put_IsStatusBarEnabled(FALSE);
                            }

                            const std::wstring webFolder = GetExecutableDirectory() + L"\\web";
                            const DWORD attrs = GetFileAttributesW(webFolder.c_str());
                            if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                                MessageBoxW(hwnd,
                                    L"Os arquivos da interface não foram encontrados.\n\n"
                                    L"Compile o projeto novamente para gerar a pasta web ao lado do executável.",
                                    L"Otimizador", MB_OK | MB_ICONERROR);
                                return E_FAIL;
                            }

                            result = g_webView->SetVirtualHostNameToFolderMapping(
                                L"app.local", webFolder.c_str(),
                                COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
                            if (FAILED(result)) {
                                ShowWebViewError(result);
                                return result;
                            }

                            EventRegistrationToken token{};
                            result = g_webView->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [hwnd](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        LPWSTR rawMessage = nullptr;
                                        const HRESULT readResult = args->TryGetWebMessageAsString(&rawMessage);
                                        if (SUCCEEDED(readResult) && rawMessage) {
                                            HandleBridgeMessage(hwnd, rawMessage);
                                            CoTaskMemFree(rawMessage);
                                        }
                                        return S_OK;
                                    }).Get(), &token);
                            if (FAILED(result)) return result;

                            ResizeWebView(hwnd);
                            return g_webView->Navigate(L"https://app.local/index.html");
                        }).Get());
            }).Get());

    if (FAILED(hr))
        ShowWebViewError(hr);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        g_window = hwnd;
        InitializeWebView(hwnd);
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 980;
        info->ptMinTrackSize.y = 680;
        return 0;
    }
    case WM_SIZE:
        ResizeWebView(hwnd);
        return 0;
    case WM_APP_WEB_MESSAGE: {
        std::unique_ptr<PostedMessage> payload(reinterpret_cast<PostedMessage*>(lParam));
        if (payload) PostJson(payload->json);
        if (payload && payload->releaseBusy) g_actionBusy.store(false);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        if (g_controller) {
            g_controller->Close();
            g_controller.Reset();
        }
        g_webView.Reset();
        g_environment.Reset();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}
}

int RunMainWindow(HINSTANCE instance, int showCommand) {
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE)
        return 1;

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hIconSm = windowClass.hIcon;
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = WindowClassName;

    if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        if (SUCCEEDED(comResult)) CoUninitialize();
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0, WindowClassName, L"Otimizador — Windows 10 / 11",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1320, 860,
        nullptr, nullptr, instance, nullptr);
    if (!hwnd) {
        if (SUCCEEDED(comResult)) CoUninitialize();
        return 1;
    }

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (SUCCEEDED(comResult)) CoUninitialize();
    return static_cast<int>(message.wParam);
}
}
