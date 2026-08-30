# GPU Trace References

This directory stores GPU traces used by `.\xb.ps1 gputest`.

## Layout

```text
testdata/reference-gpu-traces/
  traces/
    *.xtr
    *.xenia_gpu_trace
  references/
    *.xtr.png
    *.xenia_gpu_trace.png
```

## Workflow

1. Capture or copy `.xtr` files into `traces/`.
2. To capture locally, run the desktop app, load a stable scene and press `F4`
   / `GPU > Trace Frame`:

```powershell
build\bin\Windows\Debug\xenia_canary.exe --trace_gpu_prefix=testdata/reference-gpu-traces/traces <game>
```

On UWP/Xbox, enable the controller hotkey in `LocalState/xenia-canary.config.toml`:

```toml
[GPU]
uwp_controller_gpu_trace = true
```

Run a title and press `LB + RB + Back + Start` once. Copy the generated `.xtr`
from `LocalState/gpu_traces/` through the Xbox Device Portal into `traces/`.

3. Generate missing reference images:

```powershell
.\xb.ps1 gputest --config=Release --no_build --generate_missing_reference_files
```

4. Run the normal diff test:

```powershell
.\xb.ps1 gputest --config=Release --no_build
```

Use Release for trace dumping when possible. If replay reports
`Trace dump failed to capture guest output`, capture another trace from a stable
menu or scene.

Image comparison requires Pillow:

```powershell
python -m pip install Pillow
```
