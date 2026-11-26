# Otimizador

Aplicativo nativo para Windows 10 e Windows 11, escrito em C++17 com Win32. O foco é ajudar quem acabou de formatar o PC a remover temporários, revisar aplicativos opcionais e instalar ferramentas úteis sem aplicar “tweaks mágicos”.

## Recursos

- **Painel do sistema:** versão/build do Windows, processador, memória RAM e espaço livre na unidade do Windows.
- **Limpeza:** apaga o conteúdo da pasta temporária do usuário (sem seguir junctions/symlinks) e oferece esvaziar a Lixeira após confirmação.
- **Debloat seletivo:** lista aplicativos opcionais conhecidos e remove somente os pacotes escolhidos para o usuário atual. Não remove Store, Edge, Defender, WebView2, componentes de atualização ou drivers.
- **Instalador:** usa o Windows Package Manager (winget) com IDs exatos para Brave, Discord, VS Code, Visual Studio 2022 Community, Git, 7-Zip, PowerToys e VLC.
- **Ferramentas da comunidade:** abre as páginas de releases do SpotifyLite e DiscordDebloat no GitHub. O app não baixa nem executa esses binários automaticamente.

## Compilar

Requisitos: Windows 10/11, Visual Studio 2022 com o workload **Desenvolvimento para desktop com C++** (MSVC v143 e Windows SDK).

1. Abra o arquivo Otimizador.sln no Visual Studio 2022.
2. Selecione **Release** e **x64**.
3. Compile com **Build > Build Solution**.

Ou execute build.bat no Windows. O executável será gerado em bin\x64\Release\Otimizador.exe.

O workflow do GitHub Actions também compila em windows-latest e publica o executável como artefato quando o build passa.

## Uso e segurança

- A limpeza de temporários atua somente na pasta temporária padrão do usuário. Arquivos em uso ou protegidos são ignorados.
- A remoção de Appx é limitada ao perfil atual; não altera pacotes provisionados para novas contas nem faz remoção em massa.
- Leia os nomes e descrições antes de selecionar apps. Alguns aplicativos podem ser úteis para você.
- A Lixeira é permanente depois de esvaziada. A tela pede confirmação antes da ação.
- Instalações via winget são iniciadas em uma janela de terminal para que você possa acompanhar e aceitar os termos.
- O Otimizador não desativa o Windows Update, Microsoft Defender, firewall, serviços essenciais, telemetria por registro ou mecanismos de segurança. Também não altera timers, planos ocultos de energia ou prioridades de processos.
- Limpeza de cache libera espaço, mas não é uma promessa de aumento de FPS. Ganhos dependem do hardware, dos programas instalados e dos gargalos reais.

## Releases externos

- [SpotifyLite - release](https://github.com/TeamAntiLag/SpotifyLite/releases/tag/SpotifyLite)
  - SHA-256 publicado pelo GitHub para o asset atualmente listado: a54dc32b0056012e29b8640541fb83a7bbc527975e0471dfa8a173accc1bc77f
- [DiscordDebloat - release v2.1](https://github.com/TeamAntiLag/DiscordDebloat/releases/tag/v2.1)
  - SHA-256 publicado pelo GitHub para o asset atualmente listado: 020c7956eef9f12c3e09fe3a71237e038136f47ae22548153457fc71bead6ee3

Essas ferramentas são projetos de terceiros. Verifique a página, o publicador e o hash do arquivo antes de executar. O hash confirma que o arquivo corresponde ao asset publicado, mas não é uma auditoria de segurança do programa.

## Estrutura

- Otimizador.sln / Otimizador.vcxproj
- build.bat
- src/main.cpp
- src/core (SystemInfo, Cleanup, PackageManager e Debloat)
- src/ui (MainWindow)
- .github/workflows/windows-build.yml

## Licença

Consulte o arquivo LICENSE.
