#!/usr/bin/env python3
import subprocess
import os

def args(arr: list[str]):
    return " ".join(arr)

def run_cmd(cmd: str):
    subprocess.run(cmd, text=True, check=True, shell=True)
    print(f"[+] {cmd}")

def patch_nop_to_jmp(buf):
    seq = b"\x90" * 5 + b"\x0f\x0b"
    idx = buf.find(seq)
    if idx == -1:
        print("[-] No 5xNOP + UD2 sequence found.")
        return buf
    jmp_from = idx + 5
    jmp_to = len(buf)
    rel_offset = jmp_to - jmp_from
    jmp_instr = b"\xE9" + rel_offset.to_bytes(4, "little")
    patched = (
        buf[:idx] +
        jmp_instr +
        b"\x90\x90" +
        buf[idx+7:]
    )
    print(f"[+] Patched JMP at offset {idx}, relative offset {rel_offset:#x}")
    return patched

CC = "x86_64-w64-mingw32-gcc-win32"
BIN_PAYLOAD_CFLAGS = args([
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

if __name__ == "__main__":
    main_base = f"src/main"
    wrapper_base = f"src/wrapper"
    payload_path = f"bin/payload.bin"

    # Compile payload C code to object file
    run_cmd(f"{CC} -c {main_base}.c -o {main_base}.o {BIN_PAYLOAD_CFLAGS}")
    run_cmd(f"{CC} -c {wrapper_base}.c -o {wrapper_base}.o {BIN_PAYLOAD_CFLAGS}")

    # Produce flat binary with payload
    run_cmd(f"ld -T utils/linker.ld {main_base}.o {wrapper_base}.o -o {payload_path}")

    # Clean up object files
    os.remove(f"{main_base}.o")
    os.remove(f"{wrapper_base}.o")
    print(f"[+] Removed intermediate object file: {main_base}.o, {wrapper_base}.o")

    # Convert flat binary into C array of bytes
    with open(payload_path, "rb") as f:
        bytes = bytearray(f.read())

    size = len(bytes)
    print(f"[+] Binary payload size: {size} bytes")

    bytes = patch_nop_to_jmp(bytes)

    print("unsigned char payload[] = {")
    for i, byte in enumerate(bytes):
        end = "" if (i + 1) == size else ","
        if i % 20 == 0 and i != 0:
            print()
        print(f"0x{byte:02x}{end} ", end="")
    print("\n};")

    size = len(bytes)
    print(f"[+] Binary payload size: {size} bytes")

