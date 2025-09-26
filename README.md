# C_To_Shellcode_NG

## 🚀 Overall Purpose

**C_To_Shellcode_NG** is a modern, modular framework for building advanced Windows shellcode from C/C++ source code.  
It enables security researchers, reverse engineers, and red teamers to write complex shellcode logic in high-level C/C++ (and even Python for tooling), compile it with familiar tools, and generate raw, position-independent shellcode binaries ready for offensive operations, testing, or research.

This project streamlines the shellcode development workflow, enabling rapid prototyping, debugging, and deployment of custom payloads for Windows environments.

---

## 📦 Features

- **Write shellcode in C/C++:** Leverage the power and flexibility of modern languages.
- **Flexible payload generation:** Output raw binaries, C arrays, or loader executables with your shellcode embedded.
- **Cross-platform build workflow:** Develop on Linux and run on Windows.
- **Dynamic API resolution:** The shellcode resolves Win32 APIs at runtime, supporting direct and forwarded exports.
- **Template-based loader:** Quickly generate and compile loader executables for testing your payloads.
- **Modular design:** Easily extend with new source modules, payload types, and utilities.

---

## 🛠️ How to Use

### 1. **Environment Setup**

- **Required OS:** Linux (recommended for building; shellcode runs on Windows)
- **Required Packages:**
  - **MinGW-w64 cross-compiler:** `x86_64-w64-mingw32-g++`
  - `ld` (GNU linker)
  - Python 3 (for the build tooling)
- **Optional:** PE inspection tools like PE-bear, dumpbin, etc.

#### **Install MinGW-w64 (Debian/Ubuntu):**
```bash
sudo apt-get update
sudo apt-get install python3 mingw-w64
```

### 2. **Project Layout**

```
C_To_Shellcode_NG/
├── src/            # C++ source files for your shellcode logic
├── utils/          # Templates (e.g., loaderTemplate.cpp), linker scripts, helper scripts
├── bin/            # Output binaries, payloads, temp files (auto-generated, ignored in git)
├── c-to-shellcode.py  # Main build script
├── README.md
├── .gitignore
...
```

### 3. **Building Shellcode**

- Write your shellcode logic in `src/main.cpp` and supporting modules.
- **For every function you want included in the shellcode, mark it with the `FUNC` macro** (this places it in a special `.func` section).
- Run the build script:
  ```bash
  python3 c-to-shellcode.py
  ```
- The script will:
  - Compile your sources to object files (using MinGW-w64 g++)
  - Link them into a raw shellcode binary (`bin/payload.bin`) using a **custom linker script** which merges `.text`, `.func`, and `.data` into a single position-independent code (PIC) shellcode blob.
  - Convert the binary to a C array and embed into a loader template
  - Compile your loader into a Windows executable (`bin/loader.exe`)
  - Clean up temporary files

### 4. **Testing Your Shellcode**

- Use `bin/loader.exe` on a Windows machine (or VM) to test your shellcode.
- The loader template marks the shellcode memory as executable and runs it in-place.

---

## 🧩 How It Works

This project is **based on [Print3M/c-to-shellcode](https://github.com/Print3M/c-to-shellcode)**, extending and modernizing its approach for contemporary use.

**The build process works as follows:**
1. **Mark functions for shellcode:** Use the `FUNC` macro to place all intended shellcode functions in the `.func` section.
2. **Compile sources:** The Python build script compiles all listed source files using MinGW-w64 for Windows x64.
3. **Custom linker script:** The linker script (`utils/linker.ld`) is used to merge the `.text`, `.func`, and `.data` sections into a single flat binary, ensuring all code and data are together and position-independent.
4. **Payload extraction:** The resulting binary is converted into a C array and embedded in a loader template.
5. **Loader compilation:** The loader is compiled into a Windows executable that allocates executable memory and runs your shellcode.

**Why this matters:**  
This method allows you to write shellcode in high-level C/C++, organize code modularly, and reliably produce position-independent shellcode for modern Windows environments.

---

## 📚 Additional Information

- **Modularity:** Add new source files in `src/` and update the `base_names` list in `c-to-shellcode.py` to include them in your build.
- **Custom payloads:** You can easily modify the loader template or payload generation process for other output formats.
- **Extensibility:** Want to support other architectures, compilers, or shellcode styles? Fork and extend!
- **Security Note:** Generated shellcode is intended for research, red teaming, or defensive testing. **Do not use for unauthorized access or harm.**

---

## 💬 Troubleshooting

- **Payload too large?** Try reducing the number of resolved APIs, or optimizing your shellcode logic.
- **Linux build errors?** Ensure MinGW-w64 and Python 3 are installed and available in your PATH.

---

## 📝 License

This project is provided for educational and research purposes.  
See the LICENSE file for details.

---

## 🤝 Contributing

Pull requests, issue reports, and suggestions are welcome!  
Help make shellcode development more accessible for everyone.

---

## 📂 Authors

- [Rotem Navon](https://github.com/RotemNavon)

---

## 🏁 Quick Start

```bash
# Install dependencies
sudo apt-get install python3 mingw-w64

# Build shellcode and loader
python3 c-to-shellcode.py

# Transfer bin/loader.exe to a Windows machine and run it!
```

---

## 🙏 Acknowledgements

Based on and inspired by [Print3M/c-to-shellcode](https://github.com/Print3M/c-to-shellcode).