# Codegen Tests

This directory contains the test assets used by the automated codegen test
runner.

Each test is structured as a source `[name].s` PPC assembly file and generated
outputs. The outputs are made using the custom build of binutils setup when
`xenia-build gentests` is called. In this fork, run `.\xb.ps1 gentests` in every
clean environment before running these tests; generated outputs under `bin/` are
local build artifacts.

Tests are run by the `xenia-cpu-ppc-tests` target, usually through
`.\xb.ps1 test --target=xenia-cpu-ppc-tests`.

## Execution

**On Xenia**: The test binary is placed into memory at `0x80000000` and all other
memory is zeroed.

All registers are reset to zero. In order to provide useful inputs tests can
specify `#_ REGISTER_IN` values.

The code is jumped into at the starting address and executed until a `blr`
instruction is encountered.

After all instructions complete any `#_ REGISTER_OUT` values are checked and if
they do not match the test is failed.

## Registers

All registers **except lr, r1, and r13** are available for usage by tests.

## Annotations

Annotations can appear at any line in a file. If a number is required it can be
in either hex or decimal form, or IEEE if floating-point.

### REGISTER_IN

```
#_ REGISTER_IN [register name] [register value]
```

Sets the value of a register prior to executing the instructions.

Examples:

```
#_ REGISTER_IN r4 0x1234
#_ REGISTER_IN r4 5678
```

### REGISTER_OUT

```
#_ REGISTER_OUT [register name] [register value]
```

Defines the expected register value when the instructions have executed. If
after all instructions have completed the register value does not match the
value given here the test will fail.

Example:

```
#_ REGISTER_OUT r3 123
```

### MEMORY_IN

```
#_ MEMORY_IN [address] [hex bytes]
```

Writes bytes to guest memory before executing the instructions.

Example:

```
#_ MEMORY_IN 10001000 00 01 02 03 04 05 06 07
```

### MEMORY_OUT

```
#_ MEMORY_OUT [address] [hex bytes]
```

Defines expected bytes in guest memory after the instructions have executed.

Example:

```
#_ MEMORY_OUT 10001000 00 01 02 03 04 05 06 07
```

`MEMORY_IN` and `MEMORY_OUT` are supported by the emulated runner. The native
PPC64 runner still treats memory annotations as unimplemented.
