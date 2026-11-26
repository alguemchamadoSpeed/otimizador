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
constexpr COLORREF Bg = RGB(9, 13, 21);
constexpr COLORREF SidebarBg = RGB(12, 18, 29);
constexpr COLORREF Panel = RGB(18, 27, 41);
constexpr COLORREF PanelHover = RGB(27, 40, 59);
constexpr COLORREF Border = RGB(38, 54, 75);
constexpr COLORREF Text = RGB(240, 244, 252);
constexpr COLORREF Muted = RGB(145, 160, 183);
constexpr COLORREF Accent = RGB(112, 132, 255);
constexpr COLORREF AccentDark = RGB(35, 47, 83);
constexpr COLORREF Green = RGB(78, 205, 158);
constexpr COLORREF Orange = RGB(245, 177, 93);

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
    RoundRect(dc, r.left, r.top, r.right, r.bottom, 16, 16);
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
    TextAt(dc, L"OTIMIZADOR  /  WINDOWS 10 • 11", 250, 20, 420, Muted, g_fontSmall);
    TextAt(dc, title, 250, 43, 760, Text, g_fontTitle);
    TextAt(dc, subtitle, 252, 84, 800, Muted, g_fontSmall);

    // Small status indicator in the page header.
    RoundPanel(dc, 854, 30, 174, 34, RGB(17, 43, 39), RGB(30, 79, 67));
    HBRUSH dot = CreateSolidBrush(Green);
    RECT dotRect{ 869, 43, 877, 51 };
    FillRect(dc, &dotRect, dot);
    DeleteObject(dot);
    TextAt(dc, L"PROTEÇÃO PRESERVADA", 884, 31, 132, Green, g_fontSmall);
}

void SidebarButton(HDC dc, int y, const wchar_t* label, Page page, const wchar_t* symbol) {
    const bool active = g_page == page;
    if (active) {
        RoundPanel(dc, 13, y, 192, 43, AccentDark, AccentDark);
        RECT indicator{ 13, y + 9, 16, y + 34 };
        Fill(dc, indicator, Accent);
    }

    RoundPanel(dc, 27, y + 10, 25, 23,
        active ? RGB(54, 69, 117) : RGB(23, 33, 49),
        active ? RGB(54, 69, 117) : RGB(23, 33, 49));
    TextAt(dc, symbol, 27, y + 8, 25, active ? RGB(226, 233, 255) : Muted,
        g_fontMedium, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    TextAt(dc, label, 64, y, 130, active ? Text : Muted, g_fontMedium);
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
    RECT edge{ SidebarWidth - 1, 0, SidebarWidth, 4000 };
    Fill(dc, edge, Border);

    // Brand block
    RoundPanel(dc, 22, 23, 44, 44, Accent, Accent);
    TextAt(dc, L"O", 22, 21, 44, RGB(255, 255, 255), g_fontTitle,
        DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    TextAt(dc, L"OTIMIZADOR", 78, 23, 122, Text, g_fontMedium);
    TextAt(dc, L"PC CARE  •  DESKTOP", 79, 47, 122, Muted, g_fontSmall);

    RECT divider{ 22, 88, 196, 89 };
    Fill(dc, divider, Border);
    TextAt(dc, L"ESPAÇO DE TRABALHO", 26, 107, 170, Muted, g_fontSmall);

    SidebarButton(dc, 136, L"Visão geral", Page::Home, L"⌂");
    SidebarButton(dc, 186, L"Limpeza", Page::Cleaner, L"✦");
    SidebarButton(dc, 236, L"Debloat seletivo", Page::Debloat, L"−");
    SidebarButton(dc, 286, L"Aplicativos", Page::Apps, L"+");
    SidebarButton(dc, 336, L"Ferramentas", Page::Tools, L"⚙");

    RECT line{ 22, 408, 196, 409 };
    Fill(dc, line, Border);
    TextAt(dc, L"FEITO PARA WINDOWS", 26, 430, 170, Green, g_fontSmall);
    WrappedText(dc,
        L"Ações transparentes e seletivas. Nenhum serviço essencial é desativado automaticamente.",
        26, 457, 166, 90, Muted, g_fontSmall);

    RoundPanel(dc, 20, 624, 178, 62, RGB(16, 25, 38), Border);
    TextAt(dc, L"VERSÃO 1.1", 34, 632, 145, Text, g_fontMedium);
    TextAt(dc, L"Windows 10 / 11  •  x64", 34, 657, 145, Muted, g_fontSmall);
}

void DrawHome(HDC dc, int contentWidth) {
    Header(dc, L"Seu Windows, mais leve.", L"Um painel simples para organizar o PC depois de uma formatação.");

    // Main hero: puts the two most-used actions first.
    RoundPanel(dc, 250, 126, contentWidth, 164, RGB(20, 31, 51), RGB(48, 67, 104));
    RoundPanel(dc, 270, 147, 4, 118, Accent, Accent);
    TextAt(dc, L"PRONTO PARA COMEÇAR?", 290, 146, 360, Muted, g_fontSmall);
    TextAt(dc, L"Cuide do seu PC em poucos passos.", 290, 173, contentWidth - 320, Text, g_fontTitle);
    WrappedText(dc,
        L"Limpe arquivos temporários, revise aplicativos pré-instalados e instale suas ferramentas favoritas.",
        292, 211, contentWidth - 330, 40, Muted, g_fontSmall);
    Button(dc, 292, 247, 178, 34, L"Abrir limpeza", true);
    Button(dc, 482, 247, 178, 34, L"Revisar aplicativos", false);

    // System overview cards
    const int gap = 13;
    const int cardWidth = (contentWidth - gap * 2) / 3;
    const int y = 310;
    const int x1 = 250;
    const int x2 = x1 + cardWidth + gap;
    const int x3 = x2 + cardWidth + gap;

    RoundPanel(dc, x1, y, cardWidth, 132);
    RoundPanel(dc, x2, y, cardWidth, 132);
    RoundPanel(dc, x3, y, cardWidth, 132);

    TextAt(dc, L"SISTEMA OPERACIONAL", x1 + 19, y + 16, cardWidth - 32, Muted, g_fontSmall);
    TextAt(dc, g_system.windowsName, x1 + 19, y + 47, cardWidth - 32, Text, g_fontTitle);
    TextAt(dc, g_system.windowsVersion, x1 + 19, y + 91, cardWidth - 32, Muted, g_fontSmall);

    TextAt(dc, L"MEMÓRIA INSTALADA", x2 + 19, y + 16, cardWidth - 32, Muted, g_fontSmall);
    TextAt(dc, g_system.memory, x2 + 19, y + 47, cardWidth - 32, Text, g_fontTitle);
    TextAt(dc, g_system.memoryDetail, x2 + 19, y + 91, cardWidth - 32, Muted, g_fontSmall);

    TextAt(dc, L"ESPAÇO LIVRE", x3 + 19, y + 16, cardWidth - 32, Muted, g_fontSmall);
    TextAt(dc, g_system.diskFree, x3 + 19, y + 47, cardWidth - 32, Text, g_fontTitle);
    TextAt(dc, g_system.diskDetail, x3 + 19, y + 91, cardWidth - 32, Muted, g_fontSmall);

    RoundPanel(dc, 250, 463, contentWidth, 83);
    TextAt(dc, L"PROCESSADOR DETECTADO", 272, 477, contentWidth - 40, Muted, g_fontSmall);
    TextAt(dc, g_system.processor, 272, 505, contentWidth - 48, Text, g_fontMedium,
        DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    TextAt(dc, L"FLUXO RECOMENDADO", 250, 568, contentWidth, Muted, g_fontSmall);
    const int half = (contentWidth - 14) / 2;
    RoundPanel(dc, 250, 594, half, 82);
    RoundPanel(dc, 264 + half, 594, half, 82);
    TextAt(dc, L"01  •  Remova arquivos temporários", 270, 602, half - 34, Text, g_fontMedium);
    WrappedText(dc, L"Libere espaço sem tocar em pastas do sistema.", 270, 630, half - 34, 34, Muted, g_fontSmall);
    TextAt(dc, L"02  •  Revise os apps pré-instalados", 284 + half, 602, half - 34, Text, g_fontMedium);
    WrappedText(dc, L"Escolha manualmente o que realmente não utiliza.", 284 + half, 630, half - 34, 34, Muted, g_fontSmall);
}

void DrawCleaner(HDC dc, int contentWidth) {
    Header(dc, L"Limpeza inteligente", L"Confira o que será removido antes de executar cada ação.");

    const int gap = 16;
    const int half = (contentWidth - gap) / 2;
    const int rightX = 250 + half + gap;

    RoundPanel(dc, 250, 132, half, 316);
    RoundPanel(dc, rightX, 132, half, 316);

    RoundPanel(dc, 272, 153, 42, 42, AccentDark, AccentDark);
    TextAt(dc, L"✦", 272, 151, 42, RGB(226, 232, 255), g_fontMedium,
        DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    TextAt(dc, L"ARQUIVOS TEMPORÁRIOS", 328, 153, half - 95, Muted, g_fontSmall);
    TextAt(dc, FormatBytes(g_tempBytes), 274, 221, half - 42, Text, g_fontTitle);
    TextAt(dc, L"Tamanho estimado no perfil atual", 276, 263, half - 48, Muted, g_fontSmall);
    RECT sep1{ 274, 300, 250 + half - 22, 301 };
    Fill(dc, sep1, Border);
    TextAt(dc, L"Local: %LOCALAPPDATA%\\Temp", 276, 316, half - 45, Text, g_fontSmall);
    WrappedText(dc, L"Arquivos em uso, protegidos e links simbólicos são ignorados.",
        276, 344, half - 48, 42, Muted, g_fontSmall);
    Button(dc, 276, 394, half - 52, 38, L"Limpar temporários", true);

    RoundPanel(dc, rightX + 22, 153, 42, 42, RGB(55, 43, 30), RGB(87, 65, 38));
    TextAt(dc, L"↻", rightX + 22, 151, 42, Orange, g_fontMedium,
        DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    TextAt(dc, L"LIXEIRA DO WINDOWS", rightX + 78, 153, half - 100, Muted, g_fontSmall);
    TextAt(dc, L"Recupere espaço", rightX + 24, 221, half - 44, Text, g_fontTitle);
    WrappedText(dc, L"Os itens da Lixeira serão removidos permanentemente. Confira antes de continuar.",
        rightX + 26, 270, half - 52, 66, Muted, g_fontSmall);
    RECT sep2{ rightX + 24, 350, rightX + half - 22, 351 };
    Fill(dc, sep2, Border);
    TextAt(dc, L"Ação permanente • exige confirmação", rightX + 26, 363,
        half - 48, Orange, g_fontSmall);
    Button(dc, rightX + 26, 394, half - 52, 38, L"Esvaziar Lixeira", false);

    RoundPanel(dc, 250, 470, contentWidth, 88, RGB(16, 25, 38), Border);
    TextAt(dc, L"ESTADO DA ÚLTIMA OPERAÇÃO", 272, 482, contentWidth - 44, Muted, g_fontSmall);
    TextAt(dc, g_status, 272, 511, contentWidth - 44, Green, g_fontMedium);
    TextAt(dc, L"Dica: feche programas que estejam usando arquivos temporários antes da limpeza.",
        250, 578, contentWidth, Muted, g_fontSmall);
}

void DrawDebloat(HDC dc, int contentWidth) {
    Header(dc, L"Debloat seletivo", L"Você escolhe. Nenhum aplicativo é removido sem marcar e confirmar.");

    const auto& apps = core::GetOptionalApps();
    unsigned int selectedCount = 0;
    for (std::size_t i = 0; i < apps.size(); ++i)
        if (g_debloatSelected[i]) ++selectedCount;

    RoundPanel(dc, 250, 119, contentWidth, 39, RGB(18, 31, 48), Border);
    TextAt(dc, L"APLICATIVOS OPCIONAIS", 268, 119, 250, Muted, g_fontSmall);
    TextAt(dc, std::to_wstring(selectedCount) + L" selecionado(s)",
        250 + contentWidth - 190, 119, 170, Accent, g_fontMedium,
        DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

    for (std::size_t i = 0; i < apps.size(); ++i) {
        const int y = 170 + static_cast<int>(i) * 47;
        RoundPanel(dc, 250, y, contentWidth, 42,
            g_debloatSelected[i] ? RGB(24, 36, 58) : Panel,
            g_debloatSelected[i] ? Accent : Border);

        RECT check{ 267, y + 12, 285, y + 30 };
        Fill(dc, check, g_debloatSelected[i] ? Accent : RGB(31, 43, 60));
        if (g_debloatSelected[i]) {
            HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            HGDIOBJ old = SelectObject(dc, pen);
            MoveToEx(dc, 271, y + 21, nullptr);
            LineTo(dc, 276, y + 25);
            LineTo(dc, 282, y + 16);
            SelectObject(dc, old);
            DeleteObject(pen);
        }

        TextAt(dc, apps[i].name, 300, y + 1, contentWidth - 330, Text, g_fontMedium);
        TextAt(dc, apps[i].description, 300, y + 20, contentWidth - 330, Muted, g_fontSmall);
    }

    Button(dc, 250, 558, 228, 39, L"Remover selecionados", true, selectedCount == 0);
    TextAt(dc, g_status, 496, 558, contentWidth - 248, Muted, g_fontSmall);
    TextAt(dc, L"Os apps são removidos apenas do usuário atual. A reinstalação pode depender da Microsoft Store.",
        250, 614, contentWidth, Muted, g_fontSmall);
}

void DrawApps(HDC dc, int contentWidth) {
    Header(dc, L"Biblioteca de aplicativos", L"Instale ferramentas conhecidas pelo Windows Package Manager.");

    const bool winget = core::IsWingetAvailable();
    RoundPanel(dc, 250, 119, contentWidth, 46,
        winget ? RGB(16, 39, 35) : RGB(48, 35, 23),
        winget ? RGB(37, 83, 68) : RGB(94, 68, 38));
    TextAt(dc, winget ? L"●  WINGET DISPONÍVEL  •  pronto para instalar"
                      : L"●  WINGET NÃO ENCONTRADO  •  atualize o App Installer pela Microsoft Store",
        270, 119, contentWidth - 40, winget ? Green : Orange, g_fontSmall);

    const auto& packages = core::GetPackages();
    const int gap = 14;
    const int cardWidth = (contentWidth - gap) / 2;
    for (std::size_t i = 0; i < packages.size(); ++i) {
        const int col = static_cast<int>(i % 2);
        const int row = static_cast<int>(i / 2);
        const int x = 250 + col * (cardWidth + gap);
        const int y = 181 + row * 76;

        RoundPanel(dc, x, y, cardWidth, 66);
        RoundPanel(dc, x + 13, y + 16, 34, 34,
            RGB(34, 46, 71), RGB(48, 64, 96));
        TextAt(dc, L"↗", x + 13, y + 14, 34, Accent, g_fontMedium,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER);
        TextAt(dc, packages[i].name, x + 58, y + 7, cardWidth - 172, Text, g_fontMedium);
        TextAt(dc, packages[i].description, x + 58, y + 32, cardWidth - 172, Muted, g_fontSmall);
        Button(dc, x + cardWidth - 101, y + 16, 88, 34, L"Instalar", true, !winget);
    }

    TextAt(dc, g_status, 250, 586, contentWidth, Muted, g_fontSmall);
}

void DrawTools(HDC dc, int contentWidth) {
    Header(dc, L"Ferramentas extras", L"Atalhos para configurações úteis do próprio Windows e projetos externos.");

    const int half = (contentWidth - 14) / 2;
    const int rightX = 264 + half;

    RoundPanel(dc, 250, 130, half, 145);
    TextAt(dc, L"01  /  INICIALIZAÇÃO", 272, 151, half - 40, Muted, g_fontSmall);
    TextAt(dc, L"Controle o que inicia", 272, 181, half - 40, Text, g_fontMedium);
    WrappedText(dc, L"Revise os aplicativos que iniciam junto com o Windows.",
        272, 210, half - 42, 38, Muted, g_fontSmall);
    Button(dc, 272, 237, 174, 32, L"Abrir configurações", true);

    RoundPanel(dc, rightX, 130, half, 145);
    TextAt(dc, L"02  /  PROGRAMAS", rightX + 22, 151, half - 40, Muted, g_fontSmall);
    TextAt(dc, L"Aplicativos instalados", rightX + 22, 181, half - 40, Text, g_fontMedium);
    WrappedText(dc, L"Use a tela oficial do Windows para alterar ou remover programas.",
        rightX + 22, 210, half - 42, 38, Muted, g_fontSmall);
    Button(dc, rightX + 22, 237, 174, 32, L"Abrir aplicativos", false);

    TextAt(dc, L"PROJETOS DA COMUNIDADE", 250, 316, contentWidth, Muted, g_fontSmall);

    RoundPanel(dc, 250, 344, half, 132);
    TextAt(dc, L"SpotifyLite", 272, 362, half - 40, Text, g_fontMedium);
    WrappedText(dc, L"Projeto de terceiros da TeamAntiLag. Leia as notas do release e valide o arquivo antes de executar.",
        272, 390, half - 42, 48, Muted, g_fontSmall);
    Button(dc, 272, 441, 174, 30, L"Ver release", true);

    RoundPanel(dc, rightX, 344, half, 132);
    TextAt(dc, L"DiscordDebloat v2.1", rightX + 22, 362, half - 40, Text, g_fontMedium);
    WrappedText(dc, L"Ferramenta de terceiros para personalizar o Discord. Confira as notas e a compatibilidade.",
        rightX + 22, 390, half - 42, 48, Muted, g_fontSmall);
    Button(dc, rightX + 22, 441, 174, 30, L"Ver release", false);

    RoundPanel(dc, 250, 496, contentWidth, 88);
    TextAt(dc, L"SEGURANÇA E MANUTENÇÃO", 272, 512, contentWidth - 40, Muted, g_fontSmall);
    TextAt(dc, L"Defender, firewall e Windows Update permanecem sob controle do Windows.",
        272, 541, contentWidth - 225, Text, g_fontSmall);
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
        if (y >= 136 && y < 179) g_page = Page::Home;
        else if (y >= 186 && y < 229) g_page = Page::Cleaner;
        else if (y >= 236 && y < 279) g_page = Page::Debloat;
        else if (y >= 286 && y < 329) g_page = Page::Apps;
        else if (y >= 336 && y < 379) g_page = Page::Tools;

        if (g_page == Page::Cleaner) g_tempBytes = core::GetTemporaryFilesSize();
        if (g_page == Page::Home) RefreshSystem();
        InvalidateRect(g_window, nullptr, TRUE);
        return;
    }

    const int contentWidth = std::max(450, clientWidth - SidebarWidth - 42);
    if (g_page == Page::Home) {
        if (y >= 247 && y <= 281) {
            if (x >= 292 && x <= 470) {
                g_page = Page::Cleaner;
                g_tempBytes = core::GetTemporaryFilesSize();
            } else if (x >= 482 && x <= 660) {
                g_page = Page::Debloat;
            }
            InvalidateRect(g_window, nullptr, TRUE);
        }
    } else if (g_page == Page::Cleaner) {
        const int half = (contentWidth - 16) / 2;
        const int rightX = 250 + half + 16;
        if (x >= 276 && x <= 276 + half - 52 && y >= 394 && y <= 432) {
            RunCleanup();
        } else if (x >= rightX + 26 && x <= rightX + half - 26 && y >= 394 && y <= 432) {
            RunRecycleBin();
        }
    } else if (g_page == Page::Debloat) {
        const auto& apps = core::GetOptionalApps();
        for (std::size_t i = 0; i < apps.size(); ++i) {
            const int rowY = 170 + static_cast<int>(i) * 47;
            if (x >= 250 && x <= 250 + contentWidth && y >= rowY && y <= rowY + 42) {
                g_debloatSelected[i] = !g_debloatSelected[i];
                InvalidateRect(g_window, nullptr, TRUE);
                return;
            }
        }
        if (x >= 250 && x <= 478 && y >= 558 && y <= 597) RunDebloat();
    } else if (g_page == Page::Apps) {
        const auto& packages = core::GetPackages();
        const int gap = 14;
        const int cardWidth = (contentWidth - gap) / 2;
        for (std::size_t i = 0; i < packages.size(); ++i) {
            const int col = static_cast<int>(i % 2);
            const int row = static_cast<int>(i / 2);
            const int cardX = 250 + col * (cardWidth + gap);
            const int cardY = 181 + row * 76;
            if (x >= cardX + cardWidth - 105 && x <= cardX + cardWidth - 8 &&
                y >= cardY + 10 && y <= cardY + 58) {
                if (!core::IsWingetAvailable()) {
                    ShowMessage(L"winget não está disponível.\\n\\nAbra a Microsoft Store e atualize o App Installer.");
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
        const int rightX = 264 + half;
        if (x >= 272 && x <= 446 && y >= 237 && y <= 270)
            OpenUrl(L"ms-settings:startupapps");
        else if (x >= rightX + 22 && x <= rightX + 196 && y >= 237 && y <= 270)
            OpenUrl(L"ms-settings:appsfeatures");
        else if (x >= 272 && x <= 446 && y >= 441 && y <= 475)
            OpenUrl(L"https://github.com/TeamAntiLag/SpotifyLite/releases/tag/SpotifyLite");
        else if (x >= rightX + 22 && x <= rightX + 196 && y >= 441 && y <= 475)
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
        g_fontTitle = CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        RefreshSystem();
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 1020;
        info->ptMinTrackSize.y = 720;
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
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1240, 820,
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
