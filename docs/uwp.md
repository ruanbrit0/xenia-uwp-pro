# UWP / Xbox

Este documento descreve o fluxo local deste fork UWP. O build desktop herdado do Xenia Canary continua documentado em `docs/building.md`.

## Versao Atual

A base estavel atual e `1.1.13` (`1.1.13.0` no manifesto). Esta versao mantem o decoder XMA antigo no UWP e foi a que apresentou melhor resultado pratico ate agora no Xbox com um titulo retail de streaming pesado. Ainda ha lags e FPS baixo; compatibilidade e desempenho continuam experimentais.

## Requisitos

- Windows 10/11 x64.
- Visual Studio 2022 Community ou superior com workload C++.
- Windows SDK `10.0.22621.0` ou compativel.
- Python 3.8+ no `PATH`.
- Git com submodulos habilitados.
- NuGet/restauracao de pacotes para `Microsoft.Windows.CppWinRT.2.0.250303.1`, se `build/packages/` ainda nao existir.

O build local validado usa Visual Studio 2022 Community `17.14.39` e Windows SDK `10.0.22621.0`.

## Setup Em Uma Maquina Nova

Clone o repositorio e rode o setup pela raiz:

```powershell
git clone --recursive https://github.com/ruanbrit0/xenia-uwp-pro.git
cd xenia-uwp-pro
.\xb.ps1 setup
```

Se o clone ja foi feito sem `--recursive`, o `setup` atualiza os submodulos e roda Premake. Para regenerar somente os projetos depois de mudancas no build system:

```powershell
.\xb.ps1 premake
```

Nao chame `xb` diretamente no Windows. Use sempre `.\xb.ps1 <comando>` neste repositorio.

## Build UWP

O projeto UWP fica em `xenia-canary-uwp/` e referencia bibliotecas geradas em `build/`. Rode `.\xb.ps1 setup` ou `.\xb.ps1 premake` antes de buildar em ambiente limpo.

Build por MSBuild:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "xenia-canary-uwp\xenia-canary-uwp.vcxproj" /nologo /m /v:m /p:Configuration=Debug /p:Platform=x64
```

Build pelo Visual Studio:

```text
Open Project/Solution -> xenia-canary-uwp/xenia-canary-uwp.vcxproj
Configuration -> Debug
Platform -> x64
Startup Project -> xenia-canary-uwp
Target -> Local Machine
```

O pacote Debug de teste e gerado em:

```text
xenia-canary-uwp\AppPackages\xenia-canary-uwp\xenia-canary-uwp_1.1.13.0_Debug_Test\
```

Artefatos em `xenia-canary-uwp/AppPackages/`, `xenia-canary-uwp/x64/` e `xenia-canary-uwp/xenia-canary-uwp/` sao saida local de build e nao devem ser commitados.

## Deploy No Xbox

Use um console em Developer Mode. Depois do build, instale o `.appxbundle` gerado na pasta `AppPackages` usando o Device Portal do Xbox ou o deploy do Visual Studio.

Se o pacote for recusado por certificado, confirme que o certificado instalado bate com o `Publisher` do manifesto. O manifesto atual usa `CN=SirMangler` e o certificado temporario do projeto deve bater com esse valor.

O manifesto nao usa `runFullTrust`. Ele ainda declara `broadFileSystemAccess`, `removableStorage` e `codeGeneration`, que podem afetar instalacao ou execucao em ambientes UWP/Xbox mais restritos.

## Notas De Configuracao

- `Debug|x64` do projeto UWP referencia dependencias Premake `Debug Windows-UWP|x64`.
- Nao troque essas referencias para `Debug Windows|x64`; o build UWP precisa de `XE_PLATFORM_WINRT=1`.
- O Debug UWP usa runtime Release compativel com as libs geradas (`/MD`, `_ITERATOR_DEBUG_LEVEL=0`). Mudar para runtime Debug reintroduz conflito `LNK2038`.
- Se `Microsoft.Windows.CppWinRT.2.0.250303.1` faltar em `build/packages/`, restaure os pacotes NuGet antes de buildar o projeto UWP.

## Mudancas UWP Mantidas Neste Fork

- Fluxo de inicializacao UWP e selecao/resolucao de caminhos pelo frontend.
- Protecoes para APIs sensiveis em WinRT, incluindo debug break, excecoes e chamadas que nao se comportam como Win32 desktop.
- Ajustes de montagem VFS para conteudo instalado e caches locais.
- Leituras XContent/SVOD protegidas por lock por arquivo, evitando corrida em `Seek` + `fread` quando o mesmo `FILE*` e compartilhado.
- No UWP, XMA nao usa thread dedicada por padrao; frames/offsets invalidos no decoder antigo sao tratados como drop controlado com flush do FFmpeg.
- Implementacoes e stubs adicionais em `xam`/`xboxkrnl` para reduzir falhas de boot.
- Melhorias em resolucao de conteudo, arquivos, perfil de usuario, entrada e chamadas de I/O usadas por jogos retail.

## Testes E Verificacao

Build desktop padrao:

```powershell
.\xb.ps1 build
```

Testes padrao, quando o ambiente local estiver confiavel:

```powershell
.\xb.ps1 test
```

Para validar UWP, a verificacao mais importante e instalar o pacote no alvo real e coletar logs do app. Se o log parar logo depois do `CONFIG DUMP`, confira o `log_level` da configuracao local antes de concluir que nao houve falha em runtime.

Logs locais copiados do Xbox podem ficar em `logsxbox/`. Essa pasta e ignorada pelo Git e nao deve ser commitada.
