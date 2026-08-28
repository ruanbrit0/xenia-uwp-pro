# Notas Para Agentes

## Estrutura Do Repo
- Fork C++17 do Xenia Canary para UWP/Xbox; projetos raiz sao gerados por `xenia-build`/Premake em `build/`.
- Nao edite arquivos gerados em `build/`; altere `premake5.lua` na raiz ou no subdiretorio correto.
- App desktop alvo e `xenia-app`, com entrada em `src/xenia/app/xenia_main.cc`.
- App UWP fica em `xenia-canary-uwp/` e referencia `build/*.vcxproj`; rode setup/Premake antes de buildar UWP em ambiente limpo.
- Prototipo Android fica em `android/android_studio_project`; Gradle espera `build/xenia.Application.mk` e `build/xenia.wks.Android.mk`.

## Comandos
- No Windows, use `.\xb.ps1 <comando>`; nao rode `xb` diretamente porque pode abrir como texto.
- Setup inicial: `.\xb.ps1 setup` atualiza submodulos e roda Premake.
- Regenerar projetos: `.\xb.ps1 premake`.
- Build desktop gerado: `.\xb.ps1 build`.
- Testes padrao: `.\xb.ps1 test`; alvos padrao sao `xenia-base-tests` e `xenia-cpu-ppc-tests`.
- Teste focado: `.\xb.ps1 test --target=xenia-cpu-ppc-tests -- instr_foo`.
- Teste sem rebuild: `.\xb.ps1 test --no_build --target=xenia-base-tests -- --success`.
- Formatar alteracoes desde `HEAD`: `.\xb.ps1 format`; checar com `.\xb.ps1 lint`.
- Lint completo: `.\xb.ps1 lint --all`; estilo PR: `.\xb.ps1 lint --origin`.
- Evite `.\xb.ps1 pull` sem pedido explicito: ele troca para `master`, puxa/rebaseia, atualiza submodulos e reroda Premake.

## UWP Local
- Build UWP local validado com Visual Studio 2022 Community `17.14.39` e SDK Windows `10.0.22621.0`.
- MSBuild direto: `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" "xenia-canary-uwp\xenia-canary-uwp.vcxproj" /nologo /m /v:m /p:Configuration=Debug /p:Platform=x64`.
- Para rodar no Visual Studio: abrir `xenia-canary-uwp/xenia-canary-uwp.vcxproj`, selecionar `Debug | x64 | Local Machine`, definir `xenia-canary-uwp` como startup project e pressionar `F5`.
- Pacote gerado fica em `xenia-canary-uwp/AppPackages/xenia-canary-uwp/xenia-canary-uwp_1.1.14.0_Debug_Test/`.
- O projeto espera `Microsoft.Windows.CppWinRT.2.0.250303.1` em `build/packages/`; restaure com NuGet se faltar.
- As referencias UWP mapeiam `Debug|x64` para dependencias Premake `Debug Windows-UWP|x64`; nao troque isso para `Debug Windows|x64`, pois UWP precisa de `XE_PLATFORM_WINRT=1`.
- O Debug UWP usa runtime Release compativel com as libs geradas (`/MD`, `_ITERATOR_DEBUG_LEVEL=0`); mudar para runtime Debug reintroduz `LNK2038`.
- `xenia-canary-uwp_TemporaryKey.pfx` foi renovado com `CN=SirMangler`; se expirar ou for substituido, o `Publisher` do manifesto precisa bater com o certificado.
- `runFullTrust` foi removido do manifesto para evitar `APPX0006`; `broadFileSystemAccess` continua como capability restrita e ainda pode afetar deploy em ambientes UWP/Xbox mais fechados.

## Gotchas De Codigo UWP
- `XE_PLATFORM_WINRT` deve ficar desligado por padrao em `src/xenia/base/platform.h` e ser ligado apenas pela plataforma Premake `Windows-UWP`.
- `src/xenia/vfs/devices/xcontent_container_file.cc` protege `Seek` + `fread` com mutex por `FILE*`; nao remova isso, pois evita corrida em streaming XContent/SVOD.
- No UWP, `src/xenia/apu/xma_decoder.cc` nao usa thread dedicada de XMA por padrao, e `xma_context_old.cc` descarta frames/offsets invalidos de forma controlada. Nao reative a thread dedicada no UWP sem validar em Xbox.
- `src/xenia/ui/imgui_drawer.cc` deve usar somente a API nova de input ImGui (`io.AddKeyEvent`); nao reintroduza `io.KeyMap`, pois isso causa assert em runtime.
- `src/xenia/ui/file_picker_win.cc` precisa da definicao completa de `Win32Window`; mantenha o include de `xenia/ui/window_win.h` se o cast para `Win32Window` continuar ali.
- O loader XEX/XDL deve carregar imports de usuario e garantir `DLL_PROCESS_ATTACH` antes de `DLL_THREAD_ATTACH`; nao volte a chamar `DllMain` de DLL guest em thread host.

## Assets Gerados
- Rode `.\xb.ps1 buildshaders` depois de alterar `.glsl`, `.hlsl` ou `.xesl`.
- `buildshaders --target=dxbc` exige Windows e FXC do Windows SDK esperado pelo `xenia-build`.
- `buildshaders --target=spirv` exige `VULKAN_SDK` com `glslangValidator`, `spirv-opt`, `spirv-remap` e `spirv-dis`.
- Rode `.\xb.ps1 gentests` depois de alterar testes PPC `instr_*.s` ou `seq_*.s`; binarios gerados ficam versionados em `src/xenia/cpu/ppc/testing/bin/`.

## Build System
- Premake roda com `--test-suite-mode=combined`; arquivos `_test.cc` normalmente viram binarios de suite.
- Arquivos de plataforma sao escolhidos por sufixo via `local_platform_files`: `_win`, `_posix`, `_linux`, `_android`.
- Prefira arquivos com sufixo de plataforma a grandes blocos `#ifdef`.
- Mantenha codigo especifico do emulador fora de `xenia-base`; ela e uma biblioteca leve de compatibilidade.

## Estrategia De Compatibilidade
- A tag `1.1.13` segue sendo o baseline validado para Forza Horizon no Xbox UWP; use-a como ponto de comparacao antes de aceitar mudancas globais arriscadas.
- O objetivo do fork e evoluir para um emulador multifuncional para varios jogos, sem transformar correcoes em hacks especificos de titulo.
- Otimizacoes novas devem ser pensadas como melhorias gerais de APU, VFS, kernel, GPU, timing ou configuracao UWP, com mudancas pequenas e testaveis.
- Alteracoes em APU/XMA, XContent/SVOD, threading, timing, shader/cache ou filesystem devem ser retestadas contra o baseline do Forza quando houver risco de regressao.
- Se uma mudanca for experimental ou puder afetar compatibilidade existente, prefira uma flag/config para permitir desligar o comportamento sem perder o restante do trabalho.
- Para novos jogos-alvo, colete sintomas e logs reais antes de alterar codigo; documente quando uma decisao depender de comportamento observado em jogo.

## Estilo E Politica
- C++ usa `.clang-format`: base Google, ponteiros a esquerda, includes ordenados, blocos de includes preservados.
- Estilo do projeto: 2 espacos, LF, C++ em 80 colunas e TODOs atribuidos como `TODO(nome):`.
- Nao adicione codigo/comentarios derivados de XDKs vazados ou proprietarios.
- Para interfaces/protocolos guest, documente fontes de engenharia reversa ou contexto de reproducao quando a corretude depender disso.
- Evite hacks especificos de jogos ou identificadores de jogos visiveis na UI.

## Artefatos Locais
- `log/` pode existir apenas como anotacao local; nao commitar sem pedido explicito.
- `logsxbox/` e pasta local para logs/configs copiados do Xbox; fica ignorada no Git e nao deve ser commitada.
- `xenia-canary-uwp/x64/`, `xenia-canary-uwp/xenia-canary-uwp/` e `xenia-canary-uwp/AppPackages/` sao artefatos de build; nao commitar sem intencao explicita.

## Fluxo de commit e release (obrigatorio)
- Em todo commit, escolha o tipo pelo que realmente mudou: `feat`, `fix`, `docs`, `test`, `build`, `perf`, `style`, `refactor`, `chore`, `ci`, `raw`, `cleanup`, `remove`.
- Formato da mensagem de commit: `<tipo>: <descricao curta> [<versao>]`.
- A mensagem de commit deve ser escrita em portugues.
- Apos cada commit enviado (push), criar e enviar uma nova tag Git com versionamento semantico.
- Regras de incremento de versao: patch para mudancas pequenas (exemplo `1.0.0` -> `1.0.1`), minor para novos recursos sem quebra, major para mudancas com quebra ou alto impacto.
- Antes do commit, atualizar tambem a versao da aplicacao em `xenia-canary-uwp/Package.appxmanifest` e referencias documentadas relacionadas, usando a mesma decisao de incremento semantico.
- Sempre retornar um relatorio curto apos o push com: hash/mensagem do commit, nova tag, arquivos alterados e o que foi adicionado/alterado.
