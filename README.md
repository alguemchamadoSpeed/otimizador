# Otimizador

Aplicativo para Windows 10 e Windows 11 com interface grafica em PowerShell 5.1/WinForms e inicializador BAT. O repositorio tambem mantem a implementacao nativa C++17 como versao legada. O foco e ajudar quem acabou de formatar o PC a limpar temporarios, revisar aplicativos opcionais e instalar ferramentas conhecidas sem aplicar "tweaks magicos".

## Iniciar (recomendado)

1. Baixe ou clone o repositorio.
2. Deixe `Otimizador.bat` e `Otimizador.ps1` na mesma pasta.
3. Execute `Otimizador.bat` com duplo clique.

O `build.bat` antigo tambem encaminha para o novo inicializador, por compatibilidade. A interface PowerShell nao exige Visual Studio, MSBuild, Node.js ou bibliotecas externas. O BAT usa o Windows PowerShell 5.1 que acompanha o Windows 10/11 e aplica `ExecutionPolicy Bypass` somente ao processo iniciado, sem alterar a politica permanente do computador.

## Recursos

- **Visao geral:** versao/build do Windows, processador, memoria RAM e espaco livre na unidade do Windows.
- **Limpeza:** apaga o conteudo da pasta temporaria do usuario (sem seguir junctions/symlinks) e oferece esvaziar a Lixeira apos confirmacao.
- **Debloat seletivo:** lista aplicativos opcionais conhecidos e remove somente os pacotes selecionados para o usuario atual. Nao remove Store, Edge, Defender, WebView2, componentes de atualizacao ou drivers.
- **Instalador:** usa o Windows Package Manager (winget) com IDs fixos para Brave, Firefox, Discord, VS Code, Git, 7-Zip, PowerToys, VLC, Spotify e Visual Studio 2022 Community.
- **Ferramentas:** abre as configuracoes oficiais de inicializacao, aplicativos instalados, Seguranca do Windows e Windows Update.
- **Projetos externos:** abre as paginas de releases do SpotifyLite e DiscordDebloat no GitHub. O app nao baixa nem executa esses binarios automaticamente.

## Versao C++ legada (opcional)

A implementacao nativa Win32/C++17 permanece no repositorio. Para compila-la manualmente, use Visual Studio 2022 com o workload **Desenvolvimento para desktop com C++** (MSVC v143 e Windows SDK):

1. Abra `Otimizador.sln` no Visual Studio 2022.
2. Selecione **Release** e **x64**.
3. Compile com **Build > Build Solution**.

Essa etapa e opcional e nao e necessaria para executar `Otimizador.bat`.

## Uso e seguranca

- A limpeza de temporarios atua somente em `%LOCALAPPDATA%\Temp`. Arquivos em uso ou protegidos sao ignorados; links simbolicos e junctions nao sao percorridos.
- A remocao de Appx e limitada ao perfil atual; nao altera pacotes provisionados para novas contas nem faz remocao em massa.
- Leia os nomes e descricoes antes de selecionar apps. Alguns aplicativos podem ser uteis para voce.
- A Lixeira e permanente depois de esvaziada. A tela pede confirmacao antes da acao.
- Instalacoes via winget sao iniciadas em uma janela de terminal para que voce possa acompanhar e aceitar os termos.
- O Otimizador nao desativa o Windows Update, Microsoft Defender, firewall, servicos essenciais ou mecanismos de seguranca.
- Limpeza de cache libera espaco, mas nao e uma promessa de aumento de FPS. Ganhos dependem do hardware, dos programas instalados e dos gargalos reais.

## Releases externos

- [SpotifyLite - release](https://github.com/TeamAntiLag/SpotifyLite/releases/tag/SpotifyLite)
- [DiscordDebloat - release v2.1](https://github.com/TeamAntiLag/DiscordDebloat/releases/tag/v2.1)

Essas ferramentas sao projetos de terceiros. Verifique a pagina, o publicador e o arquivo antes de executar.

## Estrutura

- `Otimizador.bat` - inicializador
- `Otimizador.ps1` - interface e funcoes em PowerShell
- `build.bat` - alias de compatibilidade para o inicializador
- `Otimizador.sln` / `Otimizador.vcxproj` - versao C++ legada
- `src/core` e `src/ui` - fontes C++ legadas
- `.github/workflows/windows-build.yml` - verificacao PowerShell e build C++ no CI

## Licenca

Consulte o arquivo LICENSE.
