# Xenia Canary UWP - frontend for the Xbox One/Series

Xenia Canary UWP is an unofficial fork of Xenia Canary to support UWP and the Xbox platforms. It is not associated with the Xenia developers.

## Como Rodar Localmente

Este fork tem foco no app UWP/Xbox em `xenia-canary-uwp/`. Para rodar localmente no Visual Studio, abra `xenia-canary-uwp/xenia-canary-uwp.vcxproj`, selecione `Debug | x64 | Local Machine`, defina `xenia-canary-uwp` como projeto inicial e pressione `F5`.

Em um checkout limpo, rode `./xb.ps1 setup` uma vez para atualizar submodulos e gerar os projetos em `build/`. O guia geral de build desktop/Premake fica em `docs/building.md`; no Windows deste repo, use `./xb.ps1 <comando>` em vez de `xb` diretamente.

Build UWP direto por MSBuild:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "xenia-canary-uwp\xenia-canary-uwp.vcxproj" /nologo /m /v:m /p:Configuration=Debug /p:Platform=x64
```

O pacote gerado fica em:

```text
xenia-canary-uwp\AppPackages\xenia-canary-uwp\xenia-canary-uwp_1.1.6.0_Debug_Test\
```

Notas locais de build e manutencao UWP estao em `AGENTS.md`.

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
