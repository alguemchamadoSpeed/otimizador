#include "Debloat.hpp"
#include <Windows.h>
#include <algorithm>
#include <vector>

namespace core {
const std::vector<OptionalApp>& GetOptionalApps() {
    static const std::vector<OptionalApp> apps = {
        { L"Microsoft.BingNews", L"Microsoft Notícias", L"Notícias e conteúdo editorial" },
        { L"Microsoft.BingWeather", L"Clima", L"Previsão do tempo" },
        { L"Microsoft.GetHelp", L"Obter Ajuda", L"Aplicativo de suporte da Microsoft" },
        { L"Microsoft.Getstarted", L"Dicas do Windows", L"Dicas e introdução ao Windows" },
        { L"Microsoft.MicrosoftSolitaireCollection", L"Microsoft Solitaire", L"Jogos de cartas pré-instalados" },
        { L"Microsoft.People", L"Pessoas", L"Agenda de contatos legada" },
        { L"Microsoft.Todos", L"Microsoft To Do", L"Lista de tarefas; remova somente se não usar" },
        { L"Microsoft.WindowsFeedbackHub", L"Hub de Feedback", L"Envio de comentários para a Microsoft" }
    };
    return apps;
}

bool RemoveCurrentUserApp(const std::wstring& packageId, std::wstring& message) {
    const auto& allowlist = GetOptionalApps();
    const auto found = std::find_if(allowlist.begin(), allowlist.end(),
        [&](const OptionalApp& app) { return app.id == packageId; });
    if (found == allowlist.end()) {
        message = L"Pacote não permitido. Nenhuma alteração foi feita.";
        return false;
    }

    // Package ID comes from a fixed allowlist; no arbitrary PowerShell input is accepted.
    std::wstring script = L"$ErrorActionPreference='Stop'; $p=Get-AppxPackage -Name '" +
        found->id + L"'; if ($null -eq $p) { exit 2 }; $p | Remove-AppxPackage; exit 0";
    std::wstring command = L"powershell.exe -NoLogo -NoProfile -NonInteractive -WindowStyle Hidden -Command \"" +
        script + L"\"";

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};

    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');

    if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
        message = L"Não foi possível iniciar o PowerShell.";
        return false;
    }

    const DWORD wait = WaitForSingleObject(process.hProcess, 90000);
    DWORD exitCode = 1;
    if (wait == WAIT_OBJECT_0)
        GetExitCodeProcess(process.hProcess, &exitCode);
    else
        TerminateProcess(process.hProcess, 1);

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    if (wait == WAIT_TIMEOUT) {
        message = L"Tempo limite excedido. Verifique se o aplicativo ainda está em uso.";
        return false;
    }
    if (exitCode == 0) {
        message = L"Pacote removido do usuário atual.";
        return true;
    }
    if (exitCode == 2) {
        message = L"Esse aplicativo não está instalado para o usuário atual.";
        return false;
    }

    message = L"Falha ao remover o pacote. Ele pode estar protegido ou em uso.";
    return false;
}
}
