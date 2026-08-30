# Testing

This document describes the test flow used by this UWP fork. On Windows, run
commands through `.\xb.ps1`; do not call `xb` directly.

## Recommended Flow

From the repository root, prepare generated PPC test binaries before running the
default test set:

```powershell
.\xb.ps1 gentests
.\xb.ps1 test
```

Run `gentests` in every clean environment before running `xenia-cpu-ppc-tests`.
The generated files live under `src/xenia/cpu/ppc/testing/bin/` and are local
build artifacts, not files assumed to exist in a fresh checkout.

## Test Commands

Run the default automated tests:

```powershell
.\xb.ps1 test
```

Run tests without rebuilding first:

```powershell
.\xb.ps1 test --no_build
```

Run one target:

```powershell
.\xb.ps1 test --target=xenia-base-tests
```

Pass arguments to the test executable after `--`:

```powershell
.\xb.ps1 test --target=xenia-base-tests -- --success
```

Run one PPC assembly test suite:

```powershell
.\xb.ps1 test --target=xenia-cpu-ppc-tests -- instr_add
```

Continue after a failing test target:

```powershell
.\xb.ps1 test --continue
```

## Test Suites

See `docs/testes_criados.md` for the detailed list of test suites and coverage
added in this fork update.

| Suite | Default | Type | Notes |
| --- | --- | --- | --- |
| `xenia-apu-tests` | No | Catch2 | APU helpers such as XMA packet parsing/register metadata. |
| `xenia-base-tests` | Yes | Catch2 | Base utilities and platform helpers. |
| `xenia-cpu-ppc-tests` | Yes | Custom PPC runner | Requires `.\xb.ps1 gentests` first in clean environments. |
| `xenia-cpu-tests` | No | Catch2 | CPU helper and vector tests. |
| `xenia-kernel-tests` | No | Catch2 | Kernel/XAM primitives such as overlapped completion. |
| `xenia-vfs-tests` | No | Catch2 | VFS tests. |

The default `.\xb.ps1 test` command runs only `xenia-base-tests` and
`xenia-cpu-ppc-tests`. Use `--target` for the other suites.

## PPC Codegen Tests

PPC tests are stored as assembly files in `src/xenia/cpu/ppc/testing/` with
names such as `instr_add.s` or `seq_branch_carry.s`.

Generate their binary outputs with:

```powershell
.\xb.ps1 gentests
```

`gentests` uses the bundled PowerPC binutils from
`third_party/binutils-ppc-cygwin/` to generate local `.o`, `.dis`, `.bin` and
`.map` outputs under `src/xenia/cpu/ppc/testing/bin/`.

The emulated PPC runner loads each generated binary at `0x80000000`, discovers
labels named `test_*`, executes each test until return, then checks annotations
in the source assembly file.

Supported annotations in the emulated runner:

```text
#_ REGISTER_IN <register> <value>
#_ REGISTER_OUT <register> <value>
#_ MEMORY_IN <address> <hex bytes>
#_ MEMORY_OUT <address> <hex bytes>
```

Example:

```asm
test_add:
  #_ REGISTER_IN r4 1
  #_ REGISTER_IN r5 2
  add r3, r4, r5
  #_ REGISTER_OUT r3 3
  blr
```

## GPU Trace Diff Tests

GPU trace diff testing is available through:

```powershell
.\xb.ps1 gputest
```

This command builds and runs `xenia-gpu-d3d12-trace-dump` through
`tools/gpu-trace-diff`, using traces and references from
`testdata/reference-gpu-traces/`. Results are written to
`build/gputest/results.html`.

`tools/gpu-trace-diff` runs on Python 3. Image comparison requires Pillow:

```powershell
python -m pip install Pillow
```

Useful options:

```powershell
.\xb.ps1 gputest --no_build
.\xb.ps1 gputest --generate_missing_reference_files
.\xb.ps1 gputest --update_reference_files
```

If `testdata/reference-gpu-traces/traces/` is not present, `gputest` cannot run
until trace files are provided. Current captures use `.xtr`; older
`.xenia_gpu_trace` files are also accepted. Reference images live under
`testdata/reference-gpu-traces/references/`; missing references can be generated
from existing traces with `--generate_missing_reference_files`.

On UWP/Xbox, enable the controller hotkey in `LocalState/xenia-canary.config.toml`:

```toml
[GPU]
uwp_controller_gpu_trace = true
```

The UWP app writes traces to `LocalState/gpu_traces/` by default. While a title is
running, press `LB + RB + Back + Start` once to request a single-frame trace,
then copy the generated `.xtr` from the Xbox Device Portal into
`testdata/reference-gpu-traces/traces/` on the PC.

To capture a single-frame trace locally, run the desktop app with the trace
prefix pointed at the test trace directory, load the game, reach a stable scene
and press `F4` / `GPU > Trace Frame`:

```powershell
build\bin\Windows\Debug\xenia_canary.exe --trace_gpu_prefix=testdata/reference-gpu-traces/traces <game>
```

Then generate references and rerun the diff:

```powershell
.\xb.ps1 gputest --config=Release --no_build --generate_missing_reference_files
.\xb.ps1 gputest --config=Release --no_build
```

Use Release for trace dumping when possible. Debug trace dump builds may stop on
debug-only assertions during teardown before the image comparison finishes.

If replay loads the trace but prints `Trace dump failed to capture guest output`,
the trace is not useful as a visual reference. Capture another frame after the
game has reached a stable menu or scene, not during loading, transitions, fade
frames or immediately after opening the title.

## UWP / Xbox Verification

Automated tests validate desktop/core behavior. They do not replace UWP/Xbox
validation.

For UWP changes, also build the UWP project, install the package on the target
device when needed, run the affected game, and collect `xenia.log`, the active
config and `recent.toml`. If a runtime log stops immediately after
`CONFIG DUMP`, check the local `log_level` before assuming there was no runtime
failure.

Retest carefully when changing APU, XMA, XContent/SVOD, VFS, filesystem,
threading, timing, GPU, shader/cache, loader XEX/XDL or XAM profile/save code.
For this fork, Forza on the validated `1.1.13` baseline remains the main
regression comparison for high-risk UWP compatibility changes.

Use `docs/xbox_360_optimization_notes.md` as a checklist for performance
triage. For FPS or stutter, test without heavy logs, compare 480p and 720p,
toggle GPU readbacks only one at a time, and separate GPU, XMA, VFS/STFS/SVOD
and XAM/profile symptoms before changing code.

## Checklist De Diagnostico Por Jogo

1. Registrar Title ID, formato da midia, config local ativa e sintoma exato.
2. Desligar logs pesados antes de medir performance: `flush_log`, `log_fps`,
   `log_draw_stats` e `log_viz_query_stats`.
3. Usar 720p (`internal_display_resolution = 8`) como baseline.
4. Para FPS baixo ou stutter, comparar com 480p
   (`internal_display_resolution = 0`) antes de alterar codigo.
5. Para texto/menu invisivel, render-to-texture ou efeitos ausentes, testar
   `d3d12_readback_resolve = true` isoladamente.
6. Para suspeita de shader memory export consumido pelo CPU, testar
   `d3d12_readback_memexport = true` isoladamente.
7. Para suspeita de precisao de render target, testar
   `render_target_path_d3d12 = "rov"` quando o hardware suportar ROV.
8. Se uma flag corrigir o jogo, preferir config por jogo em vez de mudar padrao
   global, e documentar FPS antes/depois.
9. Se nada corrigir, coletar log/trace e classificar a suspeita principal como
   GPU/EDRAM, CPU/JIT, APU/XMA, VFS/STFS/SVOD ou XAM/storage.

## CI Notes

The Linux CI builds and runs `xenia-base-tests`. It excludes the timer-related
tests `Wait on Timer`, `Wait on Multiple Timers` and `HighResolutionTimer` due
to known instability tracked upstream.
