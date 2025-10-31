# C_To_Shellcode_NG

## 🚀 Purpose

**C_To_Shellcode_NG** is a modern framework for building advanced Windows shellcode from C/C++.  
Write your payload logic in high-level code, compile it on Linux, and generate raw position-independent shellcode for red teaming, research, or testing.

---

## 🏁 Quick Start

```bash
sudo apt-get install python3 mingw-w64
python3 c-to-shellcode.py
# Transfer bin/loader.exe to Windows and run!
```

---

## 📦 Features

- **Write shellcode in C/C++** — develop complex payloads in a familiar language.
- **Flexible output** — produce raw binaries, C arrays, or ready-to-run loader executables.
- **Cross-platform build** — compile on Linux, execute on Windows.
- **Dynamic WinAPI resolution** — handles direct and forwarded exports at runtime.
- **Vectored Exception Handler (VEH)** — automatic exception handling wrapper around user payload logic.
- **Modular & extensible** — add source files, payload types, or utilities easily.

---

## 🛠️ Usage

### 1. **Environment**

- **Build OS:** Linux
- **Required:** Python 3, MinGW-w64 cross-compiler (`x86_64-w64-mingw32-g++-win32`)

### 2. **Project Structure**

```
C_To_Shellcode_NG/
├── src/            # C++ shellcode sources
├── utils/          # Templates, linker scripts
├── bin/            # Output files (gitignored)
├── examples/       # Example shellcode sources
├── c-to-shellcode.py
...
```

### 3. **Build**

- Run:
  ```bash
  python3 c-to-shellcode.py
  ```
- The script will:
  - Compile sources (MinGW-w64).
  - Link them into a raw shellcode blob (merges `.text`, `.func`, `.data`, `.bss`).
  - Patch inline hook markers (optional, [see Inline Hook Patching](#️-inline-hook-patching-sandwich-method)).
  - Convert the binary to a C byte array and embed in the loader template.
  - Compile the loader (`bin/<given_name>loader.exe`).
  - Clean up temp files.

### 4. **Test**

- Run `bin/loader.exe` on Windows/VM to execute your shellcode (the loader marks memory executable and calls payload).

---

## 🧩 How It Works

The framework generates position-independent shellcode by compiling C++ code without dependencies and merging sections into a single flat binary:

1. **Compile without dependencies**: Sources are compiled with `-nostdlib`, `-nostartfiles`, and `-ffreestanding` flags, producing object files with no external dependencies.

2. **Custom linker script**: A custom `ld` script (`utils/linker.ld`) merges the `.text`, `.func`, `.data`, and `.bss` sections into a single contiguous position-independent code (PIC) block.

3. **Flat binary output**: The result is a raw binary blob where all code and data exist in one continuous memory space.

4. **Embed in loader**: The binary is converted to a C array and embedded in a loader template, which is compiled into a Windows executable that executes the shellcode.

---

## 🖼️ Shellcode Structure

Below is the memory layout of the final shellcode binary:

```
+-------------------------------+
|    Entry Point (.text)        |  ← StartWrapper function only
+-------------------------------+
|    Shellcode Functions (.func)|  ← All FUNC-marked functions
+-------------------------------+
|    Strings, Values, Globals   |  ← Constants, literals, GLOBAL_VAR variables
|        (.data + .bss)         |
+-------------------------------+
```

---

## 🗃️ Using Global Variables

Globals are fully supported in shellcode.  
There are two cases:

**1. Single-source-file global**  
Declare as `GLOBAL_VAR` directly in the `.cpp` file:

```cpp
// main.cpp
GLOBAL_VAR int my_counter = 0;
```

**2. Multi-source-files global**  
Declare as `GLOBAL_VAR` in a shared header (`.h`):

```cpp
// wrapper.h
GLOBAL_VAR DYNAMIC_FUNCTIONS g_functions = {};
```

Access from any file by including the header:

```cpp
// main.cpp
#include "wrapper.h"
// Use: g_functions.ShellExecuteA(...)
```

Always initialize globals before use. In flat binaries, uninitialized globals may contain garbage.

---

## 🛠️ Header Usage

Any header file can be imported in your shellcode source files, **but only for using macros, structs, enums, and type definitions**.  
**Do not use or import functions from headers as it wont compile**

---

## 🛠️ ⚠️ CRT vs WinAPI Heap Functions

### Problem: CRT Functions and Shellcode

When writing shellcode or position-independent code, using standard C runtime (CRT) heap functions like `malloc`, `calloc`, `realloc`, and `free` can cause issues:

- These functions are located in `msvcrt.dll`, which may not always be loaded or available depending on Windows configuration.
- Using X-macros or dynamic resolution for these symbols in C++ can cause compilation problems due to type mismatches and missing declarations.
- In many cases, the C runtime is not initialized or available in the shellcode environment.

### Solution: Use Native WinAPI Heap Functions!

Instead of CRT functions, use the Windows API equivalents from `kernel32.dll`:

| CRT Function | WinAPI Equivalent       | DLL          |
| ------------ | ----------------------- | ------------ |
| malloc       | `HeapAlloc`             | kernel32.dll |
| calloc       | `HeapAlloc` (with flag) | kernel32.dll |
| realloc      | `HeapReAlloc`           | kernel32.dll |
| free         | `HeapFree`              | kernel32.dll |

#### Example Usage:

```cpp
// Replace all uses of malloc/calloc/realloc/free with WinAPI equivalents:
HANDLE hHeap = g_functions.GetProcessHeap();
char* buffer = (char*)g_functions.HeapAlloc(hHeap, HEAP_ZERO_MEMORY, bufferSize);
// For realloc:
buffer = (char*)g_functions.HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, buffer, newSize);
// For free:
g_functions.HeapFree(hHeap, 0, buffer);
```

---

## 🪝 Inline Hook Patching (Sandwich Method)

### What is Inline Hooking?

Inline hooking allows you to redirect execution from an existing binary to your shellcode and back. Instead of replacing large chunks of code, you use the **sandwich method**:

```
[Target Binary at offset X]
    ↓ (12-byte absolute JMP)
[Allocated Memory with Shellcode]
├─ Overwritten bytes (original 12 bytes from offset X)
├─ Your shellcode logic
└─ JMP back to offset X+12 (resume normal execution)
```

**Why 12 bytes?**  
```
movabs rax, <x64 address> : 0x48, 0xB8, ??, ??, ??, ??, ??, ??, ??, ??  -> 10 bytes  
jmp rax                   : 0xff, 0xe0                                  -> 2 bytes
```

### How It Works

1. **Allocate memory** for your shellcode in the target process
2. **Copy the first 12 bytes** from offset X into your shellcode (to preserve original instructions)
3. **Write your shellcode logic** in the middle
4. **Add a JMP back** to offset X+12 to resume normal program flow
5. **Overwrite offset X** in the target binary with a 12-byte absolute jump to your allocated shellcode

This creates a "sandwich" where execution flows: Original Code → Your Shellcode → Back to Original Code

### Building for Inline Hooks

Use the `--inline` flag when building:

```bash
python3 c-to-shellcode.py --inline
```

This enables the `INLINE_HOOK_MODE` define, which:

- Adds register preservation (push/pop all registers)
- Allocates shadow space (`sub $0x32, %rsp`)
- Adds a NOP and 2 UD2 bytes at the end as a marker for automatic patching

The build script's `patch_inline_hook` function automatically:
- Locates the NOP + 2xUD2 marker (`0x90 0x0f 0x0b 0x0f 0x0b`) at the shellcode end
- Replaces it with a 5-byte relative JMP to the end of the shellcode
- This JMP serves as the "return point" where you can add your own absolute jump back to the target binary

### Implementation Requirements

When using `--inline` mode:

- The entry function (`StartWrapper`) should preserve all registers and allocate shadow space
- At the shellcode start, include the 12 overwritten bytes from the target binary
- At the end, add your own absolute jump back to offset X+12 in the target binary
- The automatic patching handles the exit jump placeholder

---

## 📚 Notes & Troubleshooting

- Add new modules in `src/` and update `base_names` in the Python script.
- For custom payloads, edit the loader template or Python build logic.
- **Large payloads:** Optimize logic and minimize WinAPI usage.
- **Build errors:** Check MinGW-w64 and Python 3 installation.
- **Security:** For research or defense only. Do not use for unauthorized access.

---

## 📝 License

See LICENSE for details.

---

## 🤝 Contributing

Contributions and suggestions welcome!

---

## 📂 Author

- [Rotem Navon](https://github.com/RotemNavon)

---

## 🙏 Acknowledgements

Inspired by [Print3M/c-to-shellcode](https://github.com/Print3M/c-to-shellcode).
