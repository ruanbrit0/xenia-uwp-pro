# Testes Criados E Atualizados

Este documento resume a bateria de testes adicionada neste fork UWP/Xbox e o
estado de validação local.

## Resumo

Foram criadas ou ampliadas suítes automatizadas para cobrir áreas sensíveis do
fork: XAM/content/profile, VFS/XContent, APU/XMA e GPU trace tooling.

As suítes novas ou ampliadas são:

| Suite | Status | Cobertura principal |
| --- | --- | --- |
| `xenia-kernel-tests` | Criada/ampliada | XAM content, profile, devices, overlapped e enumerators. |
| `xenia-vfs-tests` | Ampliada | XContent fragmentado, EOF, concorrência e múltiplos arquivos físicos. |
| `xenia-apu-tests` | Criada | Helpers XMA, registradores XMA e contexto XMA. |
| `gputest` | Infraestrutura corrigida | Runner Python 3 e estrutura de traces/referências. |

## Kernel / XAM

Arquivos principais:

```text
src/xenia/kernel/testing/premake5.lua
src/xenia/kernel/testing/kernel_test_util.h
src/xenia/kernel/testing/overlapped_test.cc
src/xenia/kernel/testing/content_manager_test.cc
src/xenia/kernel/testing/xam_device_user_test.cc
```

Cobertura adicionada:

- `KernelStateFixture` para testes de kernel sem iniciar um emulador completo.
- Escrita de campos de `XAM_OVERLAPPED` por `CompleteOverlappedEx`.
- Sinalização de evento em conclusão overlapped.
- `ContentManager` criando, abrindo, fechando, listando e deletando conteúdo.
- `XamContentCreate`, `XamContentOpenFile` e `XamContentClose` síncronos.
- `XamContentDelete`, `XamContentGetCreator`, `XamContentSetThumbnail` e
  `XamContentGetThumbnail`.
- `XamContentResolve` e `XamContentGetDeviceVolumePath`.
- `XamContentCreateEnumerator` e `XamEnumerate` para saves.
- `XamContentAggregateCreateEnumerator` para saves no HDD.
- Negativos para conteúdo inexistente, device inválido, handle inválido e fim de
  enumeração.
- `XamContentGetLicenseMask`.
- `XamContentGetDeviceName`, `XamContentGetDeviceState`,
  `XamContentGetDeviceData` e `XamContentCreateDeviceEnumerator`.
- `XamUserGetSigninState`, `XamUserGetXUID`, `XamUserGetName`,
  `XamUserGetIndexFromXUID`, `XamUserGetSigninInfo`, `XamUserGetGamerTag` e
  `XamUserLogon`.

Mudanças de suporte necessárias:

- `KernelState::CreateForTesting(...)` para injetar `Memory`, `Processor`,
  `VirtualFileSystem` e `content_root` temporário.
- `KernelState::title_id()` retorna `0` quando não há módulo executável no
  harness de teste.
- `KernelState::GetConnectedUsers()` retorna slot 0 conectado quando não há
  `Emulator`.
- Registro XAM usa static local lazy para evitar ordem de inicialização estática
  inválida ao linkar exports diretamente nos testes.
- `X_CONTENT_DEVICE_DATA` e `X_USER_SIGNIN_INFO` foram movidos para headers para
  os testes usarem os tipos reais.
- `XamEnumerate_entry` inicializa o contador temporário como `0`.
- `xeXamContentCreate` só trata `X_ERROR_SUCCESS` como sucesso ao escrever
  header/licença, evitando aceitar erros Win32 positivos como sucesso.

Resultado validado:

```text
xenia-kernel-tests: 27 test cases, 254 assertions
```

Comando:

```powershell
.\xb.ps1 test --no_build --target=xenia-kernel-tests
```

## VFS / XContent

Arquivo principal:

```text
src/xenia/vfs/testing/vfs_test.cc
```

Cobertura adicionada:

- Leitura XContent cruzando múltiplos blocos físicos.
- Leitura parcial aparada no EOF virtual.
- Leitura zero-length.
- Leitura começando exatamente no EOF.
- Stress concorrente lendo o mesmo `FILE*` compartilhado.
- Leitura cruzando múltiplos arquivos físicos via `BlockRecord::file`.

Mudanças de suporte necessárias:

- Test peer para montar `XContentContainerEntry` sintético em teste.
- Friend declarations em `Entry` e `XContentContainerEntry` para o peer de teste.

Resultado validado:

```text
xenia-vfs-tests: 4 test cases, 77 assertions
```

Comando:

```powershell
.\xb.ps1 test --no_build --target=xenia-vfs-tests
```

## APU / XMA

Arquivos principais:

```text
src/xenia/apu/testing/premake5.lua
src/xenia/apu/testing/xma_helpers_test.cc
src/xenia/apu/testing/xma_register_file_test.cc
src/xenia/apu/testing/xma_context_test.cc
```

Cobertura adicionada:

- Parsing de metadata, frame count, skip count e first frame offset de packet
  XMA.
- Separação correta entre bits de frame count e frame offset.
- `XmaRegisterFile` inicia zerado.
- Metadata de registradores XMA conhecidos.
- Escrita/leitura independente de registradores XMA.
- Helpers de estado de input buffer em `XMA_CONTEXT_DATA`.
- Roundtrip big-endian de `XMA_CONTEXT_DATA::Store` + reload.

Resultado validado:

```text
xenia-apu-tests: 8 test cases, 54 assertions
```

Comando:

```powershell
.\xb.ps1 test --no_build --target=xenia-apu-tests
```

## CPU / PPC

Além das suítes novas, a validação manteve as suítes existentes passando.

Pontos importantes estabilizados/documentados durante o trabalho:

- O runner PPC continua carregando binários em `0x80000000`.
- Testes HIR/CPU usam `kTestFunctionAddress = 0x82000000`.
- `MUL_ADD_V128` usa `vmulps` + `vaddps`, não FMA host, para preservar o
  arredondamento esperado.
- O harness de CPU cria `ThreadState` válido antes de executar código gerado.

Comandos usados:

```powershell
.\xb.ps1 test --no_build --target=xenia-cpu-tests
.\xb.ps1 test --no_build --target=xenia-cpu-ppc-tests
```

Resultado validado para CPU Catch2:

```text
xenia-cpu-tests: 88 test cases, 876 assertions
```

`xenia-cpu-ppc-tests` retornou sem erro pelo wrapper.

## GPU Trace Tests

Arquivos principais:

```text
tools/gpu-trace-diff
testdata/reference-gpu-traces/README.md
testdata/reference-gpu-traces/traces/.gitkeep
testdata/reference-gpu-traces/references/.gitkeep
```

O que foi feito:

- `tools/gpu-trace-diff` foi portado para Python 3.
- `gputest` usa `xenia-gpu-d3d12-trace-dump` por padrão neste fork UWP/Xbox.
- Correção de prints Python 2.
- Correção de normalização de paths.
- Import de Pillow movido para o momento real de comparação de imagens.
- Erro claro quando não há trace GPU em `traces/`.
- Suporte a traces `.xtr` atuais e `.xenia_gpu_trace` antigos.
- Atalho UWP/Xbox opcional por controle para capturar um frame.
- Estrutura versionável criada para traces e referências.
- Typo `Testinging...` corrigido para `Testing...` em `xenia-build`.

Dependência:

```powershell
python -m pip install Pillow
```

Estado atual:

```text
gputest ainda depende de pelo menos um trace real, normalmente .xtr.
```

Sem traces reais, o comando falha corretamente com:

```text
ERROR: no GPU trace files (.xtr, .xenia_gpu_trace) found in testdata/reference-gpu-traces/traces
```

Fluxo desktop para completar:

```powershell
build\bin\Windows\Debug\xenia_canary.exe --trace_gpu_prefix=testdata/reference-gpu-traces/traces <game>
.\xb.ps1 gputest --no_build --generate_missing_reference_files
.\xb.ps1 gputest --no_build
```

Fluxo UWP/Xbox para capturar:

```toml
[GPU]
uwp_controller_gpu_trace = true
```

Com a flag ligada, rode um jogo no Xbox e pressione `LB + RB + Back + Start` uma
vez. O app grava o trace em `LocalState/gpu_traces/`; copie o `.xtr` pelo Xbox
Device Portal para `testdata/reference-gpu-traces/traces/` no PC e gere as
referências com `gputest` em Release:

```powershell
.\xb.ps1 gputest --config=Release --no_build --generate_missing_reference_files
.\xb.ps1 gputest --config=Release --no_build
```

Se o replay carregar o trace, mas terminar com
`Trace dump failed to capture guest output`, capture outro frame em uma cena ou
menu mais estável. Esse erro indica que o dump não conseguiu obter um framebuffer
comparável para virar referência visual.

## Validação Final Local

Comandos executados:

```powershell
.\xb.ps1 test --no_build --target=xenia-kernel-tests
.\xb.ps1 test --no_build --target=xenia-vfs-tests
.\xb.ps1 test --no_build --target=xenia-apu-tests
.\xb.ps1 test --no_build --target=xenia-base-tests
.\xb.ps1 test --no_build --target=xenia-cpu-tests
.\xb.ps1 test --no_build --target=xenia-cpu-ppc-tests
.\xb.ps1 test --no_build
```

Resultados finais:

| Suite | Resultado |
| --- | --- |
| `xenia-kernel-tests` | Passou, 27 casos / 254 asserts. |
| `xenia-vfs-tests` | Passou, 4 casos / 77 asserts. |
| `xenia-apu-tests` | Passou, 8 casos / 54 asserts. |
| `xenia-base-tests` | Passou na validação final. |
| `xenia-cpu-tests` | Passou, 88 casos / 876 asserts. |
| `xenia-cpu-ppc-tests` | Retornou sem erro pelo wrapper. |
| `gputest` | Bloqueado por falta de traces reais. |

Observação: `xenia-base-tests` teve falhas intermitentes anteriores no teste
`Wait on Timer`, mas passou na validação final.

## Limites Do Que Foi Automatizado

Não foram criados testes automatizados para:

- GPU visual real, por falta de trace `.xtr` capturado.
- Decode XMA real, por falta de samples XMA pequenos e confiáveis.
- Fluxos UWP/Xbox reais, porque dependem de ambiente WinRT/package/deploy.
- Jogos específicos, porque dependem de logs, traces ou sintomas reais.
- `CompleteOverlappedDeferredEx`, porque o worker de dispatch é iniciado pelo
  caminho normal de execução com módulo; expor isso só para teste aumentaria a
  superfície e criaria teste frágil.
