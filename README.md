# Otimizador

Aplicativo para Windows 10 e Windows 11 com núcleo nativo em C++17 e interface React no WebView2. A interface utiliza componentes locais inspirados no sistema visual shadcn/ui (Tailwind CSS, variantes e Lucide); as operações do Windows continuam no host C++.

## Interface

- React + TypeScript + Vite + Tailwind CSS v4.
- Componentes reutilizáveis no padrão shadcn/ui.
- WebView2 embarcado em uma janela Win32.
- Ponte de mensagens local: a interface só pode chamar ações permitidas pelo host.

## Recursos

- **Painel do sistema:** versão/build do Windows, processador, memória RAM e espaço livre.
- **Limpeza:** pasta temporária do usuário, sem seguir junctions/symlinks, e Lixeira com confirmação.
- **Debloat seletivo:** remove apenas pacotes opcionais conhecidos do usuário atual.
- **Instalador:** IDs exatos do Windows Package Manager para Brave, Discord, VS Code, Visual Studio 2022 Community, Git, 7-Zip, PowerToys, VLC, Firefox e Spotify.
- **Ferramentas:** atalhos de inicialização, aplicativos, Segurança do Windows e Windows Update.
- **Comunidade:** abre as páginas de releases do SpotifyLite e DiscordDebloat; arquivos de terceiros não são executados automaticamente.

## Compilar

Requisitos:
- Windows 10/11.
- Visual Studio 2022 com **Desenvolvimento para desktop com C++**, MSVC v143 e Windows SDK.
- Node.js 20 ou superior (inclui npm).
- Microsoft Edge WebView2 Runtime instalado.

### Pelo BAT

Execute `build.bat`. Ele valida Visual Studio 2022, MSVC e Node.js; baixa o NuGet CLI oficial se necessário; restaura o SDK WebView2; e compila o C++ e a interface React.

Saída:
- `bin\x64\Release\Otimizador.exe`
- `bin\x64\Release\web\` (arquivos da interface)

### Pelo Visual Studio 2022

1. Abra `Otimizador.sln`.
2. Selecione `Release | x64`.
3. Execute `build.bat` pelo menos uma vez para restaurar o pacote WebView2; depois use **Build Solution** no Visual Studio.

O SDK nativo é restaurado via NuGet `packages.config` e importado pelo target oficial do pacote. O runtime Evergreen do WebView2 é necessário no computador de execução. Se estiver ausente, instale-o pelo site oficial da Microsoft.

## Segurança

- A limpeza atua somente na pasta temporária padrão do usuário. Arquivos em uso ou protegidos são ignorados.
- O debloat é limitado a uma allowlist e ao perfil atual; não remove pacotes provisionados para novas contas.
- Leia os nomes e descrições antes de selecionar aplicativos.
- Esvaziar a Lixeira é permanente e exige confirmação.
- Instalações via winget são iniciadas em uma janela de terminal para que você possa acompanhar e aceitar os termos.
- O Otimizador não desativa Windows Update, Microsoft Defender, firewall, serviços essenciais ou mecanismos de segurança.
- Limpar cache pode liberar espaço, mas não garante aumento de FPS.

## Releases externos

- [SpotifyLite - release](https://github.com/TeamAntiLag/SpotifyLite/releases/tag/SpotifyLite)
  - SHA-256 publicado para o asset listado: `a54dc32b0056012e29b8640541fb83a7bbc527975e0471dfa8a173accc1bc77f`
- [DiscordDebloat v2.1](https://github.com/TeamAntiLag/DiscordDebloat/releases/tag/v2.1)
  - SHA-256 publicado para o asset listado: `020c7956eef9f12c3e09fe3a71237e038136f47ae22548153457fc71bead6ee3`

Essas ferramentas são projetos de terceiros. Confira a página, o publicador e o hash antes de executar. Um hash confirma a correspondência com o asset publicado, não uma auditoria de segurança.

## Estrutura

- `Otimizador.sln` / `Otimizador.vcxproj`
- `packages.config`
- `build.bat`
- `src/core`: sistema, limpeza, pacotes e debloat
- `src/ui`: janela Win32 + host WebView2
- `ui`: React, TypeScript, Vite, Tailwind e componentes shadcn/ui
- `.github/workflows/windows-build.yml`

## Licença

Consulte `LICENSE`.
