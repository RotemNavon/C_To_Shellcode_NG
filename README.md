# C_To_Shellcode_NG

## 🚀 Purpose

**C_To_Shellcode_NG** is a modern framework for building advanced Windows shellcode from C/C++.  
Write your payload logic in high-level code, compile it on Linux, and generate raw position-independent shellcode for red teaming, research, or testing.

---

## 📦 Features

- **C/C++ shellcode:** Write complex payloads in familiar languages.
- **Flexible output:** Raw binaries, C arrays, or ready-to-run loaders.
- **Cross-platform build:** Develop on Linux, run on Windows.
- **Dynamic WinAPI resolution:** Handles direct and forwarded exports at runtime.
- **Modular & extensible:** Add sources, payload types, or utilities easily.

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

- Mark shellcode functions with the `FUNC` macro (puts them in `.func`).
- Run:
  ```bash
  python3 c-to-shellcode.py
  ```
- The script:
  - Compiles sources (MinGW-w64).
  - Links them into a raw shellcode blob (merges `.text`, `.func`, `.data`).
  - Converts the binary to a C array and embeds in the loader template.
  - Compiles the loader (`bin/loader.exe`).
  - Cleans up temp files.

### 4. **Test**

- Run `bin/loader.exe` on Windows/VM to execute your shellcode (loader marks memory executable and calls payload).

---

## 🧩 How It Works

Based on [Print3M/c-to-shellcode](https://github.com/Print3M/c-to-shellcode) with modern extensions.

**Build summary:**
- Functions for shellcode go in `.func` (via `FUNC` macro).
- Python build script compiles and links to a flat PIC binary.
- Loader template executes the shellcode on Windows.

---

## 🛠️ Inline Hook Patching (Sandwich Method)

For inline hooks (`overwritten bytes - shellcode - jmp back`):

- The shellcode entry (e.g. `StartWrapper`) should use `__attribute__((naked))`.
  - **Note:** GCC may not emit 5 NOPs automatically.  
    **Manually add 5 NOPs** at the end of your function using:
  - In a naked function, **do your own stack prep**: push/pop registers, adjust RSP, etc., since the compiler does not generate prologue/epilogue.
    - Recommended: Push all registers except rsp and related, then `sub rsp, 0x32` for shadow space, and reverse at the end.
- The build script's `patch_nop_to_jmp` function:
  - Locates the NOP+UD2 marker at the end of your shellcode.
  - Replaces it with a relative JMP to the shellcode end, so you can manually append a jump back to the original code.

---

## 📚 Notes

- Add new modules in `src/` and update `base_names` in the Python script.
- For custom payloads, edit the loader template or Python build logic.
- **Security:** For research or defense only. Do not use for unauthorized access.

---

## 💬 Troubleshooting

- **Large payloads:** Optimize logic and minimize WinAPI usage.
- **Build errors:** Check MinGW-w64 and Python 3 installation.

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

## 🏁 Quick Start

```bash
sudo apt-get install python3 mingw-w64
python3 c-to-shellcode.py
# Transfer bin/loader.exe to Windows and run!
```

---

## 🙏 Acknowledgements

Inspired by [Print3M/c-to-shellcode](https://github.com/Print3M/c-to-shellcode).