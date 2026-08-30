# Xenia Canary UWP - frontend for the Xbox One/Series

Xenia Canary UWP is an unofficial fork of Xenia Canary to support UWP and the Xbox platforms. It is not associated with the Xenia developers.

## Status Do Fork

Este repositorio mantem o app UWP em `xenia-canary-uwp/` em cima do Xenia Canary, com ajustes especificos para rodar em UWP/Xbox. O foco atual e estabilizar inicializacao, acesso a arquivos, perfil de usuario, montagem de conteudo/cache e chamadas de kernel usadas por titulos retail.

A base atual e `1.3.0` (`1.3.0.0` no manifesto). Ela preserva o baseline `1.1.13` de XMA/XContent/SVOD no UWP, mantem a correcao geral no loader XEX/XDL e adiciona melhorias no frontend UWP para idioma, configuracoes e diagnostico.

O port ainda e experimental. Mesmo quando titulos retail avancam alem da introducao, ainda podem ocorrer lags, FPS baixo, travamentos, congelamentos ou loading preso durante a entrada no gameplay.

## Como Rodar O Projeto Em Outra Maquina

### Requisitos

- Windows 10/11 x64.
- Git com suporte a submodulos.
- Python 3.8+ no `PATH`.
- Visual Studio 2022 recomendado.
- Workload C++ instalado no Visual Studio.
- MSVC v143 instalado, pois o projeto UWP usa `PlatformToolset v143` por padrao.
- Windows SDK `10.0.22621.0` ou compativel.
- NuGet/restauracao de pacotes habilitada.
- Xbox em Developer Mode, caso o alvo seja instalar no console.

Ambiente validado:

```text
Visual Studio 2022 Community 17.14.39
MSVC v143
Windows SDK 10.0.22621.0
Configuration: Debug
Platform: x64
```

### Requisitos Do Visual Studio

No Visual Studio Installer, instale estas cargas de trabalho:

```text
Desenvolvimento para desktop com C++
Desenvolvimento de aplicativo WinUI
Desenvolvimento de jogos em C++
```

Em `Componentes individuais`, confirme que estes componentes estao instalados:

```text
MSVC v143 - Ferramentas de compilacao VS 2022 C++ x64/x86
Suporte a Plataforma Universal do Windows
Ferramentas da Plataforma Universal do Windows
SDK do Windows 10.0.22621.0
SDK do Windows 11 10.0.22621.0 ou superior compativel
Gerenciador de pacotes NuGet
Runtime do Windows Universal C
Ferramentas HLSL
C++ Build Insights
Ferramentas de criacao de perfil do C++
Depurador de elementos graficos e criador de perfil de GPU
Ferramentas do build ARM do C++ VS 2022
```

Tambem pode manter instalado, se ja estiver marcado:

```text
SDK do Windows 11 10.0.26100.7705
AddressSanitizer do C++
Gerenciador de pacotes do vcpkg
IntelliCode
GitHub Copilot
```

Componentes como Unreal Engine, Unity, Cocos e conectividade USB nao sao necessarios para compilar este projeto UWP, a menos que sejam usados por outro fluxo local.

### Sobre O Toolset v143

O projeto UWP usa `v143` como toolset padrao:

```xml
<PlatformToolset>v143</PlatformToolset>
```

Por isso, o caminho recomendado e usar Visual Studio 2022 com o toolset MSVC v143 instalado.

O arquivo `xenia-canary-uwp/xenia-canary-uwp.vcxproj` tambem possui fallback para versoes antigas do Visual Studio:

```xml
<PlatformToolset Condition="'$(VisualStudioVersion)' == '16.0'">v142</PlatformToolset>
<PlatformToolset Condition="'$(VisualStudioVersion)' == '15.0'">v141</PlatformToolset>
<PlatformToolset Condition="'$(VisualStudioVersion)' == '14.0'">v140</PlatformToolset>
```

Na pratica:

```text
Visual Studio 2022 -> v143 recomendado
Visual Studio 2019 -> v142 fallback
Visual Studio 2017 -> v141 fallback
Visual Studio 2015 -> v140 fallback
```

Para o projeto atual, use Visual Studio 2022 com `v143`, pois esse e o ambiente validado para o build UWP atual.

### Clonar O Repositorio

Clone com submodulos:

```powershell
git clone --recursive https://github.com/ruanbrit0/xenia-uwp-pro.git
cd xenia-uwp-pro
```

Se o repositorio ja foi clonado sem `--recursive`, rode o setup inicial descrito abaixo.

### Setup Inicial

Na raiz do projeto:

```powershell
.\xb.ps1 setup
```

Esse comando atualiza submodulos e gera os projetos em `build/`.

No Windows, use sempre:

```powershell
.\xb.ps1 <comando>
```

Nao rode `xb` diretamente.

### Restaurar Pacotes NuGet

O projeto UWP espera este pacote:

```text
Microsoft.Windows.CppWinRT.2.0.250303.1
```

Ele deve existir em:

```text
build\packages\
```

Se estiver faltando, restaure os pacotes pelo Visual Studio/NuGet antes de compilar o projeto UWP.

### Regenerar Projetos

Se necessario, regenere os projetos com:

```powershell
.\xb.ps1 premake
```

Use isso depois de alteracoes em `premake5.lua` ou em configuracao de build.

### Build UWP Pelo MSBuild

O projeto UWP fica em:

```text
xenia-canary-uwp\xenia-canary-uwp.vcxproj
```

Build direto:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "xenia-canary-uwp\xenia-canary-uwp.vcxproj" /nologo /m /v:m /p:Configuration=Debug /p:Platform=x64
```

Use build incremental normalmente. Nao e necessario usar `Rebuild`.

### Build UWP Pelo Visual Studio

Abra:

```text
xenia-canary-uwp\xenia-canary-uwp.vcxproj
```

Configure:

```text
Configuration: Debug
Platform: x64
Startup Project: xenia-canary-uwp
Target: Local Machine
```

Depois pressione `F5` para executar localmente.

### Pacote Gerado

Apos o build UWP, o pacote fica em:

```text
xenia-canary-uwp/AppPackages/xenia-canary-uwp/xenia-canary-uwp_1.3.0.0_Debug_Test/
```

Bundle esperado:

```text
xenia-canary-uwp/AppPackages/xenia-canary-uwp/xenia-canary-uwp_1.3.0.0_Debug_Test/xenia-canary-uwp_1.3.0.0_x64_Debug.appxbundle
```

### Configs Padrao Por Jogo

Alguns jogos podem receber ajustes especificos de compatibilidade por config. A build UWP pode incluir configs padrao em:

```text
xenia-canary-uwp\game_configs\
```

No primeiro inicio do app, esses arquivos sao copiados para a pasta de dados do usuario, sem sobrescrever arquivos ja existentes:

```text
LocalState\config\
```

As configs sao carregadas pelo `Title ID`, entao funcionam independentemente do formato do jogo, desde que o XEX principal tenha o mesmo `Title ID`.

Esses ajustes sao aplicados somente aos jogos correspondentes e podem ser removidos ou alterados pelo usuario em `LocalState\config\`.

### Diagnostico De FPS

Para registrar FPS no `xenia.log`, ative a flag abaixo no `xenia.config.toml` global ou em uma config por jogo:

```toml
[Logging]
log_fps = true
```

O valor e calculado a partir dos swaps enviados pelo jogo e registrado periodicamente para evitar spam no log.

### Deploy No Xbox

Para instalar no Xbox, use um console em Developer Mode.

Instale o `.appxbundle` gerado usando o Xbox Device Portal ou o deploy do Visual Studio.

Se o pacote for recusado por certificado, confira se o certificado do projeto bate com o manifesto.

Manifesto atual:

```text
Publisher="CN=SirMangler"
Version="1.3.0.0"
```

### Build Desktop

Para build desktop padrao:

```powershell
.\xb.ps1 build
```

### Testes

Testes padrao:

```powershell
.\xb.ps1 test
```

Teste focado:

```powershell
.\xb.ps1 test --target=xenia-cpu-ppc-tests -- instr_nome_do_teste
```

### Formatacao E Lint

Antes de enviar alteracoes:

```powershell
.\xb.ps1 format
.\xb.ps1 lint
```

### Estrutura Importante

```text
src\
docs\
build\
xenia-canary-uwp\
```

O app UWP fica em:

```text
xenia-canary-uwp\
```

Os projetos gerados ficam em:

```text
build\
```

Nao edite arquivos gerados em `build/`. Faca alteracoes nos arquivos fonte ou nos arquivos `premake5.lua`.

### Artefatos Locais

Estes diretorios sao artefatos locais de build:

```text
xenia-canary-uwp\AppPackages\
xenia-canary-uwp\x64\
xenia-canary-uwp\xenia-canary-uwp\
```

### Observacoes UWP

- `Debug|x64` do projeto UWP referencia dependencias `Debug Windows-UWP|x64`.
- Nao troque as referencias para `Debug Windows|x64`.
- UWP precisa de `XE_PLATFORM_WINRT=1`.
- O Debug UWP usa runtime Release compativel com as libs geradas.
- Runtime esperado: `/MD` com `_ITERATOR_DEBUG_LEVEL=0`.
- Mudar para runtime Debug pode causar erro `LNK2038`.
- O manifesto nao usa `runFullTrust`.
- O manifesto usa `broadFileSystemAccess`, `removableStorage` e `codeGeneration`.

Guia completo de setup, build, pacote e deploy UWP/Xbox: `docs/uwp.md`.

O guia geral de build desktop/Premake fica em `docs/building.md`.

Join SirMangler's UWP Discord server for help: https://discord.gg/UXVT66JSm8

Don't ask for help with this port on the official Xenia Discord server. They will not be able to help you.

The original readme and Xenia Canary repository can be found here:
https://github.com/xenia-canary/xenia-canary

## Game Compatibility

See the [Game compatibility list](https://docs.google.com/spreadsheets/d/19wHZmKcs_Mdibj8CM_M4tMB9bSidx6Se3tCLTPobuk4/edit?usp=sharing)

## Disclaimer

The goal of this project is to experiment, research, and educate on the topic
of emulation of modern devices and operating systems. **It is not for enabling
illegal activity**. All information is obtained via reverse engineering of
legally purchased devices and games and information made public on the internet
(you'd be surprised what's indexed on Google...).
