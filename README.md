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
- **Modular & extensible** — add source files, payload types, or utilities easily.

---

## 🛠️ Usage

### 1. **Environment**

- **Build OS:** Linux
- **Required:** Python 3, MinGW-w64 cross-compiler (`x86_64-w64-mingw32-g++`)
- **Install:**  
  ```bash
  sudo apt-get update
  sudo apt-get install python3 mingw-w64
  ```

### 2. **Project Structure**

```
C_To_Shellcode_NG/
├── src/            # C++ shellcode sources
├── utils/          # Templates, linker scripts
├── bin/            # Output files (gitignored)
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
  - Link them into a raw shellcode blob (merges `.text`, `.func`).
  - Patch inline hook markers (optional, see diagram below).
  - Convert the binary to a C byte array and embed in the loader template.
  - Compile the loader (`bin/loader.exe`).
  - Clean up temp files.

### 4. **Test**

- Run `bin/loader.exe` on Windows/VM to execute your shellcode (the loader marks memory executable and calls payload).

---

## 🧩 How It Works

- Functions intended for shellcode are marked with the `FUNC` macro, placing them in the `.func` section.
- The Python build script compiles all sources and links them to a flat position-independent code (PIC) binary using a custom linker script.
- The resulting binary is converted to a C array and embedded in a loader template.
- The loader template is compiled to a Windows executable that allocates memory and executes your shellcode.

---

## 🖼️ Shellcode Structure

Below is the layout of the final shellcode binary:

```
+-------------------------------+
|    Entry Point (.text)        |  (StartWrapper only)
+-------------------------------+
|    Shellcode Functions (.func)|  (All other functions, helpers, API resolvers - marked with FUNC)
+-------------------------------+
|    Strings, Values, Globals   |  (constants, literal strings, arrays, GLOBAL_VAR globals)
|        (.data + .bss)         | 
+-------------------------------+
```

- **.text**: Only the shellcode entry point (`StartWrapper`).
- **.func**: All other shellcode functions.
- **.data + .bss**: Any referenced constants, arrays, strings, and all global variables.

> The linker script merges these into a single contiguous block for position-independent execution.

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

| CRT Function | WinAPI Equivalent           | DLL           |
|--------------|----------------------------|---------------|
| malloc       | `HeapAlloc`                | kernel32.dll  |
| calloc       | `HeapAlloc` (with flag)    | kernel32.dll  |
| realloc      | `HeapReAlloc`              | kernel32.dll  |
| free         | `HeapFree`                 | kernel32.dll  |

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

## 🛠️ Inline Hook Patching (Sandwich Method)

For inline hooks (`overwritten bytes - shellcode - jmp back`):

- The shellcode entry (e.g. `StartWrapper`) should use `__attribute__((naked))`.
  - **Note:** When using `__attribute__((naked))`, GCC emits a UD2 (`0x0f, 0x0b`) instruction at the end of the function instead of the usual `ret` (`0xc3`). This serves as a marker for patching.
  - GCC may not emit NOPs automatically.  
    **Manually add 3 NOPs** at the end of your function for the patching process (since a relative jump uses 5 bytes).
  - In a naked function, **do your own stack prep**: push/pop registers, adjust `%rsp`, etc., since the compiler does not generate prologue/epilogue.
    - Recommended: Push all registers except `%rsp` and related, then `sub $0x32, %rsp` for shadow space, and reverse at the end.
- The build script's `patch_nop_to_jmp` function:
  - Locates the 3 NOPs + UD2 marker at the end of your shellcode.
  - Replaces it with a relative JMP to the shellcode end, so you can manually append a jump back to the original code.

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