#include "MainWindow.hpp"
#include "../core/SystemInfo.hpp"
#include "../core/Cleanup.hpp"
#include "../core/PackageManager.hpp"
#include "../core/Debloat.hpp"

#include <Windows.h>
#include <windowsx.h>
#include <Shellapi.h>
#include <ShlObj.h>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")

namespace ui {
namespace {
enum class Page { Home, Cleaner, Debloat, Apps, Tools };

constexpr int SidebarWidth = 218;
constexpr COLORREF Bg = RGB(12, 16, 24);
constexpr COLORREF SidebarBg = RGB(15, 21, 31);
constexpr COLORREF Panel = RGB(21, 29, 42);
constexpr COLORREF PanelHover = RGB(27, 38, 54);
constexpr COLORREF Border = RGB(39, 50, 68);
constexpr COLORREF Text = RGB(238, 242, 249);
constexpr COLORREF Muted = RGB(148, 161, 181);
constexpr COLORREF Accent = RGB(104, 137, 255);
constexpr COLORREF AccentDark = RGB(39, 57, 104);
constexpr COLORREF Green = RGB(77, 196, 145);
constexpr COLORREF Orange = RGB(240, 174, 91);

HWND g_window = nullptr;
Page g_page = Page::Home;
core::SystemSnapshot g_system;
std::uint64_t g_tempBytes = 0;
bool g_debloatSelected[8]{};
std::wstring g_status = L"Pronto. Escolha uma seção para começar.";
HFONT g_font = nullptr;
HFONT g_fontMedium = nullptr;
HFONT g_fontSmall = nullptr;
HFONT g_fontTitle = nullptr;

std::wstring FormatBytes(std::uint64_t bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) { value /= 1024.0; ++unit; }
    wchar_t buffer[80]{};
    swprintf_s(buffer, L"%.1f %s", value, units[unit]);
    return buffer;
}

void Fill(HDC dc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void RoundPanel(HDC dc, int x, int y, int width, int height,
                COLORREF color = Panel, COLORREF outline = Border) {
    RECT r{ x, y, x + width, y + height };
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, outline);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, 12, 12);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void TextAt(HDC dc, const std::wstring& text, int x, int y, int width,
            COLORREF color = Text, HFONT font = nullptr, UINT flags = DT_LEFT | DT_SINGLELINE | DT_VCENTER) {
    RECT r{ x, y, x + width, y + 50 };
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    HGDIOBJ old = nullptr;
    if (font) old = SelectObject(dc, font);
    DrawTextW(dc, text.c_str(), -1, &r, flags);
    if (old) SelectObject(dc, old);
}

void WrappedText(HDC dc, const std::wstring& text, int x, int y, int width, int height,
                 COLORREF color = Muted, HFONT font = nullptr) {
    RECT r{ x, y, x + width, y + height };
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    HGDIOBJ old = nullptr;
    if (font) old = SelectObject(dc, font);
    DrawTextW(dc, text.c_str(), -1, &r, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_END_ELLIPSIS);
    if (old) SelectObject(dc, old);
}

void Button(HDC dc, int x, int y, int width, int height, const std::wstring& label,
            bool primary = false, bool disabled = false) {
    const COLORREF background = disabled ? RGB(47, 54, 66) : (primary ? Accent : PanelHover);
    const COLORREF outline = primary ? Accent : Border;
    RoundPanel(dc, x, y, width, height, background, outline);
    TextAt(dc, label, x + 8, y + 1, width - 16,
           disabled ? Muted : Text, g_fontMedium, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
}

void Header(HDC dc, const std::wstring& title, const std::wstring& subtitle) {
    TextAt(dc, title, 250, 28, 720, Text, g_fontTitle);
    TextAt(dc, subtitle, 251, 72, 740, Muted, g_fontSmall);
}

void SidebarButton(HDC dc, int y, const wchar_t* label, Page page) {
    const bool active = g_page == page;
    if (active) RoundPanel(dc, 14, y, 190, 42, AccentDark, AccentDark);
    TextAt(dc, label, 30, y, 165, active ? Text : Muted, g_fontMedium);
}

void RefreshSystem() {
    g_system = core::GetSystemSnapshot();
}

void OpenUrl(const wchar_t* url) {
    ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

void DrawSidebar(HDC dc) {
    RECT r{ 0, 0, SidebarWidth, 4000 };
    Fill(dc, r, SidebarBg);
    RoundPanel(dc, 18, 24, 42, 42, Accent, Accent);
    TextAt(dc, L"O", 18, 24, 42, RGB(255, 255, 255), g_fontTitle, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    TextAt(dc, L"OTIMIZADOR", 70, 23, 130, Text, g_fontMedium);
    TextAt(dc, L"WINDOWS 10 / 11", 70, 47, 130, Muted, g_fontSmall);
    TextAt(dc, L"MENU PRINCIPAL", 25, 100, 170, Muted, g_fontSmall);
    SidebarButton(dc, 128, L"Visão geral", Page::Home);
    SidebarButton(dc, 176, L"Limpeza", Page::Cleaner);
    SidebarButton(dc, 224, L"Debloat seletivo", Page::Debloat);
    SidebarButton(dc, 272, L"Instalar aplicativos", Page::Apps);
    SidebarButton(dc, 320, L"Ferramentas", Page::Tools);

    RECT line{ 22, 385, 195, 386 };
    Fill(dc, line, Border);
    TextAt(dc, L"SEGURO POR PADRÃO", 25, 405, 170, Green, g_fontSmall);
    WrappedText(dc, L"Sem desativar Defender, firewall, Windows Update ou serviços essenciais.", 25, 433, 166, 68, Muted, g_fontSmall);
    TextAt(dc, L"v1.0.0  •  C++17", 25, 520, 170, Muted, g_fontSmall);
}

void DrawHome(HDC dc, int contentWidth) {
    Header(dc, L"Seu Windows, sob controle.", L"Ferramentas práticas para preparar o PC depois de uma formatação.");
    const int gap = 14;
    const int cardWidth = (contentWidth - gap * 2) / 3;
    const int y = 126;
    RoundPanel(dc, 250, y, cardWidth, 142);
    RoundPanel(dc, 250 + cardWidth + gap, y, cardWidth, 142);
    RoundPanel(dc, 250 + (cardWidth + gap) * 2, y, cardWidth, 142);

    TextAt(dc, L"SISTEMA", 270, y + 18, cardWidth - 35, Muted, g_fontSmall);
    TextAt(dc, g_system.windowsName, 270, y + 48, cardWidth - 35, Text, g_fontTitle);
    TextAt(dc, g_system.windowsVersion, 270, y + 91, cardWidth - 35, Muted, g_fontSmall);

    TextAt(dc, L"MEMÓRIA RAM", 290 + cardWidth, y + 18, cardWidth - 35, Muted, g_fontSmall);
    TextAt(dc, g_system.memory, 290 + cardWidth, y + 48, cardWidth - 35, Text, g_fontTitle);
    TextAt(dc, g_system.memoryDetail, 290 + cardWidth, y + 91, cardWidth - 35, Muted, g_fontSmall);

    TextAt(dc, L"ESPAÇO LIVRE", 290 + (cardWidth + gap) * 2, y + 18, cardWidth - 35, Muted, g_fontSmall);
    TextAt(dc, g_system.diskFree, 290 + (cardWidth + gap) * 2, y + 48, cardWidth - 35, Text, g_fontTitle);
    TextAt(dc, g_system.diskDetail, 290 + (cardWidth + gap) * 2, y + 91, cardWidth - 35, Muted, g_fontSmall);

    RoundPanel(dc, 250, 286, contentWidth, 114);
    TextAt(dc, L"PROCESSADOR", 272, 302, contentWidth - 40, Muted, g_fontSmall);
    TextAt(dc, g_system.processor, 272, 331, contentWidth - 45, Text, g_fontMedium, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    TextAt(dc, L"PRIMEIROS PASSOS", 250, 432, contentWidth, Muted, g_fontSmall);
    const int half = (contentWidth - 14) / 2;
    RoundPanel(dc, 250, 461, half, 142);
    RoundPanel(dc, 264 + half, 461, half, 142);
    TextAt(dc, L"1. Limpe arquivos temporários", 270, 479, half - 35, Text, g_fontMedium);
    WrappedText(dc, L"Remova arquivos temporários do seu perfil para recuperar espaço em disco.", 270, 510, half - 38, 42);
    Button(dc, 270, 556, 170, 34, L"Abrir limpeza", true);

    TextAt(dc, L"2. Revise aplicativos opcionais", 284 + half, 479, half - 35, Text, g_fontMedium);
    WrappedText(dc, L"Escolha individualmente quais aplicativos pré-instalados você realmente não usa.", 284 + half, 510, half - 38, 42);
    Button(dc, 284 + half, 556, 170, 34, L"Abrir debloat", false);
}

void DrawCleaner(HDC dc, int contentWidth) {
    Header(dc, L"Limpeza inteligente", L"Arquivos temporários do seu perfil. Arquivos em uso ou protegidos serão ignorados.");
    RoundPanel(dc, 250, 132, contentWidth, 205);
    TextAt(dc, L"ARQUIVOS TEMPORÁRIOS DO USUÁRIO", 274, 153, contentWidth - 45, Muted, g_fontSmall);
    TextAt(dc, FormatBytes(g_tempBytes), 274, 184, 400, Text, g_fontTitle);
    TextAt(dc, L"Local: %LOCALAPPDATA%\\Temp", 274, 230, contentWidth - 50, Muted, g_fontSmall);
    TextAt(dc, L"A limpeza não percorre atalhos, junctions ou links simbólicos.", 274, 253, contentWidth - 50, Muted, g_fontSmall);
    Button(dc, 274, 287, 220, 36, L"Limpar temporários", true);

    RoundPanel(dc, 250, 363, contentWidth, 154);
    TextAt(dc, L"LIXEIRA DO WINDOWS", 274, 383, contentWidth - 45, Muted, g_fontSmall);
    TextAt(dc, L"Remova os itens da Lixeira para liberar espaço.", 274, 414, contentWidth - 45, Text, g_fontMedium);
    WrappedText(dc, L"A ação é permanente. Confira a Lixeira antes de continuar.", 274, 445, contentWidth - 50, 35);
    Button(dc, 274, 468, 180, 36, L"Esvaziar Lixeira", false);

    TextAt(dc, g_status, 250, 550, contentWidth, Green, g_fontSmall);
}

void DrawDebloat(HDC dc, int contentWidth) {
    Header(dc, L"Debloat seletivo", L"Marque somente os apps opcionais que não usa. A remoção afeta apenas seu usuário.");
    TextAt(dc, L"APLICATIVOS OPCIONAIS", 250, 116, contentWidth, Muted, g_fontSmall);

    const auto& apps = core::GetOptionalApps();
    for (std::size_t i = 0; i < apps.size(); ++i) {
        const int y = 143 + static_cast<int>(i) * 49;
        RoundPanel(dc, 250, y, contentWidth, 43, Panel, Border);
        RECT check{ 266, y + 12, 283, y + 29 };
        Fill(dc, check, g_debloatSelected[i] ? Accent : RGB(31, 41, 56));
        if (g_debloatSelected[i]) {
            HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            HGDIOBJ old = SelectObject(dc, pen);
            MoveToEx(dc, 270, y + 20, nullptr);
            LineTo(dc, 275, y + 25);
            LineTo(dc, 281, y + 15);
            SelectObject(dc, old);
            DeleteObject(pen);
        }
        TextAt(dc, apps[i].name, 298, y + 3, contentWidth - 325, Text, g_fontMedium);
        TextAt(dc, apps[i].description, 298, y + 22, contentWidth - 325, Muted, g_fontSmall);
    }
    Button(dc, 250, 552, 226, 38, L"Remover selecionados", true);
    TextAt(dc, g_status, 490, 552, contentWidth - 245, Muted, g_fontSmall);
}

void DrawApps(HDC dc, int contentWidth) {
    Header(dc, L"Instalar aplicativos", L"Instalações via Windows Package Manager (winget), com IDs exatos.");
    const bool winget = core::IsWingetAvailable();
    RoundPanel(dc, 250, 112, contentWidth, 48);
    TextAt(dc, winget ? L"winget detectado — pronto para instalar." : L"winget não encontrado. Atualize o App Installer pela Microsoft Store.",
        270, 112, contentWidth - 40, winget ? Green : Orange, g_fontSmall);

    const auto& packages = core::GetPackages();
    const int gap = 14;
    const int cardWidth = (contentWidth - gap) / 2;
    for (std::size_t i = 0; i < packages.size(); ++i) {
        const int col = static_cast<int>(i % 2);
        const int row = static_cast<int>(i / 2);
        const int x = 250 + col * (cardWidth + gap);
        const int y = 176 + row * 78;
        RoundPanel(dc, x, y, cardWidth, 68);
        TextAt(dc, packages[i].name, x + 15, y + 9, cardWidth - 125, Text, g_fontMedium);
        TextAt(dc, packages[i].description, x + 15, y + 34, cardWidth - 125, Muted, g_fontSmall);
        Button(dc, x + cardWidth - 102, y + 17, 88, 34, L"Instalar", true);
    }
    TextAt(dc, g_status, 250, 585, contentWidth, Muted, g_fontSmall);
}

void DrawTools(HDC dc, int contentWidth) {
    Header(dc, L"Ferramentas extras", L"Atalhos úteis e ferramentas opcionais da comunidade.");
    const int half = (contentWidth - 14) / 2;

    RoundPanel(dc, 250, 130, half, 145);
    TextAt(dc, L"INICIALIZAÇÃO DO WINDOWS", 272, 151, half - 40, Muted, g_fontSmall);
    TextAt(dc, L"Revise o que inicia com o PC", 272, 181, half - 40, Text, g_fontMedium);
    WrappedText(dc, L"Abre as configurações oficiais de aplicativos de inicialização.", 272, 210, half - 42, 38);
    Button(dc, 272, 237, 174, 32, L"Abrir configurações", true);

    RoundPanel(dc, 264 + half, 130, half, 145);
    TextAt(dc, L"APLICATIVOS INSTALADOS", 286 + half, 151, half - 40, Muted, g_fontSmall);
    TextAt(dc, L"Gerencie programas do PC", 286 + half, 181, half - 40, Text, g_fontMedium);
    WrappedText(dc, L"Use a página oficial do Windows para alterar ou desinstalar programas.", 286 + half, 210, half - 42, 38);
    Button(dc, 286 + half, 237, 174, 32, L"Abrir aplicativos", false);

    TextAt(dc, L"FERRAMENTAS DA COMUNIDADE", 250, 316, contentWidth, Muted, g_fontSmall);

    RoundPanel(dc, 250, 344, half, 132);
    TextAt(dc, L"SpotifyLite", 272, 362, half - 40, Text, g_fontMedium);
    WrappedText(dc, L"Projeto de terceiros da TeamAntiLag. Confira a página do release e valide o arquivo antes de executar.", 272, 390, half - 42, 48);
    Button(dc, 272, 441, 174, 30, L"Ver release", true);

    RoundPanel(dc, 264 + half, 344, half, 132);
    TextAt(dc, L"DiscordDebloat v2.1", 286 + half, 362, half - 40, Text, g_fontMedium);
    WrappedText(dc, L"Ferramenta de terceiros para personalizar o Discord. Leia as notas e confirme a compatibilidade.", 286 + half, 390, half - 42, 48);
    Button(dc, 286 + half, 441, 174, 30, L"Ver release", false);

    RoundPanel(dc, 250, 496, contentWidth, 88);
    TextAt(dc, L"SEGURANÇA E MANUTENÇÃO", 272, 512, contentWidth - 40, Muted, g_fontSmall);
    TextAt(dc, L"Abra o Windows Update ou a Segurança do Windows para revisar atualizações e proteção.", 272, 541, contentWidth - 45, Text, g_fontSmall);
    Button(dc, 250 + contentWidth - 190, 520, 168, 38, L"Segurança do Windows", false);
}

void Paint(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);
    Fill(dc, client, Bg);

    DrawSidebar(dc);
    const int contentWidth = std::max(450, static_cast<int>(client.right) - SidebarWidth - 42);

    switch (g_page) {
    case Page::Home: DrawHome(dc, contentWidth); break;
    case Page::Cleaner: DrawCleaner(dc, contentWidth); break;
    case Page::Debloat: DrawDebloat(dc, contentWidth); break;
    case Page::Apps: DrawApps(dc, contentWidth); break;
    case Page::Tools: DrawTools(dc, contentWidth); break;
    }
    EndPaint(hwnd, &ps);
}

void ShowMessage(const std::wstring& message, UINT icon = MB_ICONINFORMATION) {
    MessageBoxW(g_window, message.c_str(), L"Otimizador", MB_OK | icon);
}

void RunCleanup() {
    const auto answer = MessageBoxW(g_window,
        L"Isso removerá arquivos encontrados em %LOCALAPPDATA%\\Temp. "
        L"Arquivos em uso serão ignorados. Deseja continuar?",
        L"Confirmar limpeza", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (answer != IDYES) return;

    const auto result = core::CleanTemporaryFiles();
    g_tempBytes = core::GetTemporaryFilesSize();
    std::wostringstream report;
    report << L"Limpeza concluída.\n\nEspaço liberado: " << FormatBytes(result.bytesFreed)
           << L"\nArquivos removidos: " << result.filesRemoved
           << L"\nPastas removidas: " << result.directoriesRemoved
           << L"\nItens ignorados: " << result.skipped;
    g_status = L"Limpeza concluída. Espaço liberado: " + FormatBytes(result.bytesFreed);
    ShowMessage(report.str());
    InvalidateRect(g_window, nullptr, TRUE);
}

void RunRecycleBin() {
    const auto answer = MessageBoxW(g_window,
        L"Esvaziar a Lixeira é permanente. Confira se não há arquivos que deseja recuperar.\n\nDeseja continuar?",
        L"Confirmar ação permanente", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (answer != IDYES) return;

    if (core::EmptyRecycleBin()) {
        g_status = L"Lixeira esvaziada.";
        ShowMessage(L"A Lixeira foi esvaziada.");
    } else {
        g_status = L"Não foi possível esvaziar a Lixeira.";
        ShowMessage(g_status, MB_ICONWARNING);
    }
    InvalidateRect(g_window, nullptr, TRUE);
}

void RunDebloat() {
    std::vector<std::size_t> selected;
    const auto& apps = core::GetOptionalApps();
    for (std::size_t i = 0; i < apps.size(); ++i)
        if (g_debloatSelected[i]) selected.push_back(i);

    if (selected.empty()) {
        ShowMessage(L"Marque pelo menos um aplicativo opcional.");
        return;
    }

    std::wostringstream prompt;
    prompt << L"Remover " << selected.size() << L" aplicativo(s) do usuário atual?\n\n";
    for (const auto index : selected)
        prompt << L"• " << apps[index].name << L"\n";
    prompt << L"\nA ação pode exigir reinstalação pela Microsoft Store.";
    if (MessageBoxW(g_window, prompt.str().c_str(), L"Confirmar debloat",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return;

    unsigned int succeeded = 0;
    std::wostringstream report;
    for (const auto index : selected) {
        std::wstring message;
        if (core::RemoveCurrentUserApp(apps[index].id, message)) ++succeeded;
        report << apps[index].name << L": " << message << L"\n\n";
        g_debloatSelected[index] = false;
    }
    g_status = L"Debloat finalizado: " + std::to_wstring(succeeded) +
        L" de " + std::to_wstring(selected.size()) + L" removido(s).";
    report << L"Resultado: " << succeeded << L" de " << selected.size() << L" removido(s).";
    ShowMessage(report.str(), succeeded == selected.size() ? MB_ICONINFORMATION : MB_ICONWARNING);
    InvalidateRect(g_window, nullptr, TRUE);
}

void Click(int x, int y, int clientWidth) {
    if (x < SidebarWidth) {
        if (y >= 128 && y < 170) g_page = Page::Home;
        else if (y >= 176 && y < 218) g_page = Page::Cleaner;
        else if (y >= 224 && y < 266) g_page = Page::Debloat;
        else if (y >= 272 && y < 314) g_page = Page::Apps;
        else if (y >= 320 && y < 362) g_page = Page::Tools;
        if (g_page == Page::Cleaner) g_tempBytes = core::GetTemporaryFilesSize();
        if (g_page == Page::Home) RefreshSystem();
        InvalidateRect(g_window, nullptr, TRUE);
        return;
    }

    const int contentWidth = std::max(450, clientWidth - SidebarWidth - 42);
    if (g_page == Page::Home) {
        const int half = (contentWidth - 14) / 2;
        if (y >= 556 && y <= 593) {
            if (x >= 250 && x <= 250 + half) {
                g_page = Page::Cleaner;
                g_tempBytes = core::GetTemporaryFilesSize();
            } else if (x >= 264 + half && x <= 264 + half * 2) {
                g_page = Page::Debloat;
            }
            InvalidateRect(g_window, nullptr, TRUE);
        }
    } else if (g_page == Page::Cleaner) {
        if (x >= 270 && x <= 500 && y >= 287 && y <= 326) RunCleanup();
        else if (x >= 270 && x <= 465 && y >= 468 && y <= 507) RunRecycleBin();
    } else if (g_page == Page::Debloat) {
        const auto& apps = core::GetOptionalApps();
        for (std::size_t i = 0; i < apps.size(); ++i) {
            const int rowY = 143 + static_cast<int>(i) * 49;
            if (x >= 250 && x <= 250 + contentWidth && y >= rowY && y <= rowY + 43) {
                g_debloatSelected[i] = !g_debloatSelected[i];
                InvalidateRect(g_window, nullptr, TRUE);
                return;
            }
        }
        if (x >= 250 && x <= 480 && y >= 552 && y <= 590) RunDebloat();
    } else if (g_page == Page::Apps) {
        const auto& packages = core::GetPackages();
        const int gap = 14;
        const int cardWidth = (contentWidth - gap) / 2;
        for (std::size_t i = 0; i < packages.size(); ++i) {
            const int col = static_cast<int>(i % 2);
            const int row = static_cast<int>(i / 2);
            const int cardX = 250 + col * (cardWidth + gap);
            const int cardY = 176 + row * 78;
            if (x >= cardX + cardWidth - 108 && x <= cardX + cardWidth - 8 &&
                y >= cardY + 10 && y <= cardY + 60) {
                if (!core::IsWingetAvailable()) {
                    ShowMessage(L"winget não está disponível.\n\nAbra a Microsoft Store e atualize o App Installer.");
                } else if (core::LaunchPackageInstall(packages[i])) {
                    g_status = L"Instalador iniciado em uma janela de terminal: " + packages[i].name;
                } else {
                    g_status = L"Não foi possível iniciar o winget.";
                }
                InvalidateRect(g_window, nullptr, TRUE);
                return;
            }
        }
    } else if (g_page == Page::Tools) {
        const int half = (contentWidth - 14) / 2;
        if (x >= 270 && x <= 455 && y >= 237 && y <= 270)
            OpenUrl(L"ms-settings:startupapps");
        else if (x >= 286 + half && x <= 470 + half && y >= 237 && y <= 270)
            OpenUrl(L"ms-settings:appsfeatures");
        else if (x >= 270 && x <= 455 && y >= 441 && y <= 475)
            OpenUrl(L"https://github.com/TeamAntiLag/SpotifyLite/releases/tag/SpotifyLite");
        else if (x >= 286 + half && x <= 470 + half && y >= 441 && y <= 475)
            OpenUrl(L"https://github.com/TeamAntiLag/DiscordDebloat/releases/tag/v2.1");
        else if (x >= 250 + contentWidth - 195 && x <= 250 + contentWidth && y >= 520 && y <= 558)
            OpenUrl(L"ms-settings:windowsdefender");
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        g_window = hwnd;
        g_font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        g_fontMedium = CreateFontW(-16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        g_fontSmall = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        g_fontTitle = CreateFontW(-25, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        RefreshSystem();
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 980;
        info->ptMinTrackSize.y = 680;
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        Paint(hwnd);
        return 0;
    case WM_LBUTTONUP: {
        RECT rect{};
        GetClientRect(hwnd, &rect);
        Click(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), rect.right);
        return 0;
    }
    case WM_DESTROY:
        if (g_font) DeleteObject(g_font);
        if (g_fontMedium) DeleteObject(g_fontMedium);
        if (g_fontSmall) DeleteObject(g_fontSmall);
        if (g_fontTitle) DeleteObject(g_fontTitle);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}
}

int RunMainWindow(HINSTANCE instance, int showCommand) {
    const wchar_t* className = L"OtimizadorMainWindow";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = className;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return 1;

    HWND hwnd = CreateWindowExW(0, className, L"Otimizador — Windows 10 / 11",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1120, 760,
        nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 1;

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
}
