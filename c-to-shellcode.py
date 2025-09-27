#!/usr/bin/env python3
import subprocess
import os

# --- Global Folders ---
BIN_DIR = "bin"
UTILS_DIR = "utils"
SRC_DIR = "src"

CC = "x86_64-w64-mingw32-g++-win32"
BIN_PAYLOAD_CFLAGS = " ".join([
    "-D_MM_MALLOC_H_INCLUDED",
    "-Os",
    "-fPIC",
    "-nostdlib",
    "-nostartfiles",
    "-ffreestanding",
    "-fno-asynchronous-unwind-tables",
    "-fno-ident",
    "-e StartWrapper",
    "-s",
])

def args(arr):
    return " ".join(arr)

def run_cmd(cmd):
    subprocess.run(cmd, text=True, check=True, shell=True)
    print(f"[+] {cmd}")

def patch_nop_to_jmp(buf):
    # Search for 3 NOPs + UD2 (0x90 0x90 0x90 0x0f 0x0b)
    seq = b"\x90" * 3 + b"\x0f\x0b"
    idx = buf.find(seq)
    if idx == -1:
        print("[-] No 3xNOP + UD2 sequence found.")
        return buf
    jmp_from = idx
    jmp_to = len(buf)
    rel_offset = jmp_to - (jmp_from + 5)
    # Patch: overwrite 5 bytes with JMP rel32
    jmp_instr = b"\xE9" + rel_offset.to_bytes(4, "little")
    patched = buf[:idx] + jmp_instr + buf[idx+5:]
    print(f"[+] Patched JMP at offset {idx}, relative offset {rel_offset:#x}")
    return patched

def emit_shellcode_c_array(bytes):
    arr = []
    for i, byte in enumerate(bytes):
        if i % 20 == 0 and i != 0:
            arr.append("\n")
        arr.append(f"0x{byte:02x}, ")
    if arr and arr[-1].endswith(", "):
        arr[-1] = arr[-1][:-2] + " "
    return "unsigned char payload[] = {\n" + "".join(arr) + "\n};\n"

def write_loader_cpp(shellcode_array_code):
    template_path = f"{UTILS_DIR}/loaderTemplate.cpp"
    with open(template_path, "r") as f:
        template = f.read()
    loader_code = template.replace("SHELLCODE_BYTE_ARRAY", shellcode_array_code)
    loader_path = f"{BIN_DIR}/loader.cpp"
    with open(loader_path, "w") as f:
        f.write(loader_code)
    print(f"[+] Written loader with embedded shellcode to {loader_path}")
    return loader_path

if __name__ == "__main__":
    # List your source files (base names without extension)
    base_names = [
        f"{SRC_DIR}/main",
        f"{SRC_DIR}/wrapper",
        f"{SRC_DIR}/strUtils"
        # Add new base names here, e.g. f"{SRC_DIR}/another"
    ]
    file_ext = ".cpp"
    object_files = []

    # Compile all listed sources to object files
    for src_base in base_names:
        obj_file = f"{src_base}.o"
        run_cmd(f"{CC} -c {src_base}{file_ext} -o {obj_file} {BIN_PAYLOAD_CFLAGS}")
        object_files.append(obj_file)

    payload_path = f"{BIN_DIR}/payload.bin"

    # Produce flat binary with payload
    obj_files_str = " ".join(object_files)
    run_cmd(f"ld -T {UTILS_DIR}/linker.ld {obj_files_str} -o {payload_path}")

    # Clean up object files
    for obj_file in object_files:
        os.remove(obj_file)
        print(f"[+] Removed intermediate object file: {obj_file}")

    # Convert flat binary into C array of bytes
    with open(payload_path, "rb") as f:
        bytes = bytearray(f.read())

    size = len(bytes)
    print(f"[+] Binary payload size: {size} bytes")

    # Patch the NOPs + UD2 and write back to payload.bin
    bytes = patch_nop_to_jmp(bytes)
    with open(payload_path, "wb") as f:
        f.write(bytes)
    print(f"[+] Updated {payload_path} after patching inline hook jump.")

    shellcode_array_code = emit_shellcode_c_array(bytes)

    # Write bin/loader.cpp from utils/loaderTemplate.cpp
    loader_cpp_path = write_loader_cpp(shellcode_array_code)

    # Compile loader.cpp to loader.exe
    loader_exe_path = f"{BIN_DIR}/loader.exe"
    run_cmd(f"{CC} {loader_cpp_path} -o {loader_exe_path}")

    # Remove loader.cpp
    os.remove(loader_cpp_path)
    print(f"[+] Removed temporary loader source: {loader_cpp_path}")

    print(f"[+] Loader executable ready at {loader_exe_path}")