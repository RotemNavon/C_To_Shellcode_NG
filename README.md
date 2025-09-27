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
|    Strings and Values         |  (constants, literal strings, arrays, etc.)
+-------------------------------+
```

- **.text**: Only the shellcode entry point (`StartWrapper`).
- **.func**: All other shellcode functions (helpers, logic, API resolvers, etc.) marked with the `FUNC` macro.
- **strings/values**: Any referenced constants, arrays, and strings needed by your shellcode.

> The linker script merges these into a single contiguous block for position-independent execution.

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