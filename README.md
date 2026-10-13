# VM8080

A small **Intel 8080 CPU emulator** written in C. It loads a raw binary into a flat 64 KiB address space, executes it instruction by instruction, and dumps the final CPU state. An optional trace mode prints every executed instruction.

## Features

- **Full documented 8080 instruction set:** data transfer, arithmetic, logic, rotates, jumps, calls, returns, `RST`, stack operations, `IN`/`OUT`, `NOP`, `HLT`
- **Flags:** sign, zero, auxiliary carry, parity and carry, with `DAA` support
- **Memory:** 64 KiB flat RAM
- **Console I/O:** two ports map to stdin and stdout, so programs can print and read characters
- **Trace mode:** `--trace` prints the address and mnemonic of every executed instruction
- **Step limit:** guards against programs that never halt
- **Final state dump:** registers, flags, `SP` and `PC` are printed when execution ends
- Built with `-Wall -Wextra -Wpedantic -Werror`

## Building

### Requirements

- `clang` recent enough to accept `-std=c2y` (the Makefile uses it)
- `make`
- `clang-format` (only for `make format`)

### Commands

```sh
make          # build ./vm8080
make format   # run clang-format on include/*.h and src/*.c
make clean    # remove build output and the binary
```

## Usage

```sh
./vm8080 [--trace] [--steps N] file.bin
```

| Option        | Description                                                   |
|---------------|---------------------------------------------------------------|
| `--trace`     | Print each executed instruction                               |
| `--steps N`   | Maximum number of instructions to run (default: `1024`)       |
| `file.bin`    | Raw 8080 machine code, loaded at address `0x0000`             |

The emulator stops when the CPU executes `HLT` or when the step limit is reached. In the second case it prints a warning to stderr. On exit it prints how many instructions ran and the final register state.

Note: usage and error messages are in Portuguese.

### Example

A program that prints `Hi` and a newline, then halts:

```
0000  3E 48    MVI A, 'H'
0002  D3 11    OUT 0x11
0004  3E 69    MVI A, 'i'
0006  D3 11    OUT 0x11
0008  3E 0A    MVI A, 0x0A
000A  D3 11    OUT 0x11
000C  76       HLT
```

Build the binary and run it:

```sh
printf '\x3e\x48\xd3\x11\x3e\x69\xd3\x11\x3e\x0a\xd3\x11\x76' > hello.bin
./vm8080 hello.bin
```

Expected output:

```
Loaded 13 bytes
Hi
7 instructions executed
a=0a b=00 c=00 d=00 e=00
h=00 l=00 f=02 sp=0000 pc=000d
```

## Machine model

| Component | Details                                                              |
|-----------|----------------------------------------------------------------------|
| Registers | `A`, `B`, `C`, `D`, `E`, `H`, `L`, `F`, `SP`, `PC`                   |
| Memory    | 65,536 bytes, all writable                                           |
| Ports     | 256 one-byte ports                                                   |
| Reset     | All registers and memory zero, flags register `0x02`, `PC = 0x0000`  |

### I/O ports

| Port   | `IN`                                        | `OUT`                          |
|--------|---------------------------------------------|--------------------------------|
| `0x10` | Reads one character from stdin (`0` on EOF) | Stores the value, never read back |
| `0x11` | Always returns `0`                          | Writes the character to stdout |
| other  | Returns the last value written              | Stores the value               |

## Project layout

```
.
├── Makefile
├── include/
│   ├── alu.h           # arithmetic and logic operations, flag bits
│   ├── cpu.h           # CPU state, registers, run loop
│   ├── instruction.h   # instruction enum, decoder and executor API
│   ├── loader.h        # binary loader
│   ├── memory.h        # memory loading
│   └── vm.h            # VM initialization
└── src/
    ├── alu.c
    ├── cpu.c           # register access, fetch, step, run, state dump
    ├── instruction.c   # decode and execute, trace output
    ├── loader.c        # reads a file into memory
    ├── main.c          # command-line interface
    ├── memory.c
    └── vm.c
```

## Known limitations

- **No interrupts:** `EI` and `DI` are accepted but do nothing, and there is no interrupt input
- **No cycle counting:** execution is measured in instructions, not clock cycles
- **Console only:** there are no other peripherals
- **Undocumented opcodes** are executed as one-byte no-ops, and an invalid opcode is only reported when `--trace` is on
- **Load address:** the command line always loads at `0x0000`
- **No test suite** is included, so accuracy has not been checked against the standard 8080 test programs
- **Flag deviations from real hardware:**
  - `ANA` and `ANI` always set the auxiliary carry flag, while the real 8080 derives it from bit 3 of the operands
  - After `SUB`, `SBB`, `CMP` and `DCR` the auxiliary carry follows borrow semantics, while the real 8080 reports it inverted
  - `POP PSW` loads the flags byte as is, without forcing its fixed bits
