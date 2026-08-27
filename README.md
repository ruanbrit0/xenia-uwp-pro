# Xenia Canary UWP - frontend for the Xbox One/Series

Xenia Canary UWP is an unofficial fork of Xenia Canary to support UWP and the Xbox platforms. It is not associated with the Xenia developers.

## Status Do Fork

Este repositorio mantem o app UWP em `xenia-canary-uwp/` em cima do Xenia Canary, com ajustes especificos para rodar em UWP/Xbox. O foco atual e estabilizar inicializacao, acesso a arquivos, perfil de usuario, montagem de conteudo/cache e chamadas de kernel usadas por titulos retail.

O port ainda e experimental. Alguns jogos podem abrir menus e ainda travar, congelar ou ficar em loading durante a entrada no gameplay.

## Como Rodar Localmente

Em um checkout limpo no Windows, rode o setup uma vez para atualizar submodulos e gerar os projetos em `build/`:

```powershell
.\xb.ps1 setup
```

Para rodar o app UWP no Visual Studio, abra `xenia-canary-uwp/xenia-canary-uwp.vcxproj`, selecione `Debug | x64 | Local Machine`, defina `xenia-canary-uwp` como projeto inicial e pressione `F5`.

Build UWP direto por MSBuild:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "xenia-canary-uwp\xenia-canary-uwp.vcxproj" /nologo /m /v:m /p:Configuration=Debug /p:Platform=x64
```

O pacote gerado fica em:

```text
xenia-canary-uwp\AppPackages\xenia-canary-uwp\xenia-canary-uwp_1.1.9.0_Debug_Test\
```

Guia completo de setup, build, pacote e deploy UWP/Xbox: `docs/uwp.md`.

O guia geral de build desktop/Premake fica em `docs/building.md`. No Windows deste repo, use `.\xb.ps1 <comando>` em vez de `xb` diretamente.

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
