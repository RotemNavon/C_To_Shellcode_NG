#!/usr/bin/env python3
import subprocess
import os
import argparse
from pathlib import Path
from typing import List


class ShellcodeBuilder:
    """Builds shellcode payloads from C++ sources and generates loader executables."""
    
    NOP_UD2_SEQUENCE = b"\x90" + b"\x0f\x0b" * 2
    BYTES_PER_LINE = 20
    
    def __init__(self, inline_hook_mode: bool = False):
        self.bin_dir = Path("bin")
        self.utils_dir = Path("utils")
        self.src_dir = Path("src")
        self.compiler = "x86_64-w64-mingw32-g++-win32"
        self.inline_hook_mode = inline_hook_mode
        self.cflags = self._build_compiler_flags()
        
    def _build_compiler_flags(self) -> str:
        """Returns compiler flags for payload generation."""
        flags = [
            "-D_MM_MALLOC_H_INCLUDED",
            "-Os",
            "-fPIC",
            "-Wno-c++23-extensions",
            "-nostdlib",
            "-nostartfiles",
            "-ffreestanding",
            "-fno-asynchronous-unwind-tables",
            "-fno-ident",
            "-e StartWrapper",
            "-s",
        ]
        
        if self.inline_hook_mode:
            flags.append("-DINLINE_HOOK_MODE")
        
        return " ".join(flags)
    
    def _run_command(self, cmd: str) -> None:
        """Executes a shell command and logs it."""
        subprocess.run(cmd, text=True, check=True, shell=True)
        # print(f"[+] {cmd}")
    
    def _get_source_files(self) -> List[str]:
        """Returns list of source file base names to compile."""
        return [
            f"{self.src_dir}/main",
            f"{self.src_dir}/wrapper",
            f"{self.src_dir}/strUtils"
        ]
    
    def compile_sources(self) -> List[str]:
        """Compiles C++ sources to object files."""
        print("[+] Compiling sources")
        object_files = []
        
        for src_base in self._get_source_files():
            obj_file = f"{src_base}.o"
            cmd = f"{self.compiler} -c {src_base}.cpp -o {obj_file} {self.cflags}"
            self._run_command(cmd)
            object_files.append(obj_file)
        
        return object_files
    
    def link_payload(self, object_files: List[str], output_name: str, output_dir: Path) -> Path:
        """Links object files into a flat binary payload inside the output directory."""
        print("[+] Linking object files to binary payload")
        payload_path = output_dir / f"{output_name}.bin"
        obj_files_str = " ".join(object_files)
        linker_script = self.utils_dir / "linker.ld"
        
        cmd = f"ld -T {linker_script} {obj_files_str} -o {payload_path}"
        self._run_command(cmd)
        
        return payload_path
    
    def cleanup_object_files(self, object_files: List[str]) -> None:
        """Removes intermediate object files."""
        for obj_file in object_files:
            os.remove(obj_file)
    
    def patch_inline_hook(self, payload_bytes: bytearray) -> bytearray:
        """Patches NOP + UD2 sequence with JMP instruction to end of payload."""
        print("[+] Searching target byte sequence for patching")
        idx = payload_bytes.find(self.NOP_UD2_SEQUENCE)
        
        if idx == -1:
            print("[i] No 3xNOP + UD2 sequence found, skipping patching")
            return payload_bytes
        
        jmp_to = len(payload_bytes)
        rel_offset = jmp_to - (idx + 5)
        jmp_instr = b"\xE9" + rel_offset.to_bytes(4, "little", signed=True)
        
        patched = payload_bytes[:idx] + jmp_instr + payload_bytes[idx+5:]
        print(f"[+] Patched JMP at offset {idx}, relative offset {rel_offset:#x}")
        
        return patched
    
    def convert_to_c_array(self, payload_bytes: bytearray) -> str:
        """Converts binary payload to C array format."""
        lines = []
        current_line = []
        
        for i, byte in enumerate(payload_bytes):
            if i > 0 and i % self.BYTES_PER_LINE == 0:
                lines.append(", ".join(current_line) + ",")
                current_line = []
            current_line.append(f"0x{byte:02x}")
        
        if current_line:
            lines.append(", ".join(current_line))
        
        return "unsigned char payload[] = {\n    " + "\n    ".join(lines) + "\n};\n"
    
    def write_c_array_to_file(self, shellcode_array: str, output_name: str, output_dir: Path) -> Path:
        """Writes the C array to a .cpp file inside the output directory."""
        c_array_path = output_dir / f"{output_name}.cpp"
        with open(c_array_path, "w") as f:
            f.write(shellcode_array)
        print(f"[+] Written C array to {c_array_path}")
        return c_array_path

    def generate_loader(self, shellcode_array: str, output_name: str, output_dir: Path) -> Path:
        """Generates loader C++ file from template with embedded shellcode inside the output directory."""
        print("[+] Generating loader with embedded shellcode")
        template_path = self.utils_dir / "loaderTemplate.cpp"
        loader_path = output_dir / "loader.cpp"
        
        with open(template_path, "r") as f:
            template = f.read()
        
        loader_code = template.replace("SHELLCODE_BYTE_ARRAY", shellcode_array)
        
        with open(loader_path, "w") as f:
            f.write(loader_code)
        
        print(f"[+] Written loader with embedded shellcode to {loader_path}")
        return loader_path
    
    def compile_loader(self, loader_path: Path, output_name: str, output_dir: Path) -> Path:
        """Compiles loader C++ file to executable inside the output directory."""
        loader_exe = output_dir / f"{output_name}_loader.exe"
        cmd = f"{self.compiler} {loader_path} -o {loader_exe}"
        self._run_command(cmd)
        return loader_exe
    
    def build(self, output_name: str) -> None:
        """Orchestrates the complete build process, placing all outputs in a subfolder of bin."""
        # Create output directory inside bin
        output_dir = self.bin_dir / output_name
        output_dir.mkdir(parents=True, exist_ok=True)

        object_files = self.compile_sources()
        payload_path = self.link_payload(object_files, output_name, output_dir)
        self.cleanup_object_files(object_files)

        with open(payload_path, "rb") as f:
            payload_bytes = bytearray(f.read())

        print(f"[+] Binary payload size: {len(payload_bytes)} bytes")

        if self.inline_hook_mode:
            payload_bytes = self.patch_inline_hook(payload_bytes)

        with open(payload_path, "wb") as f:
            f.write(payload_bytes)

        shellcode_array = self.convert_to_c_array(payload_bytes)

        if self.inline_hook_mode:
            # Only output .bin and .cpp (C array)
            self.write_c_array_to_file(shellcode_array, output_name, output_dir)
            print(f"[+] Inline mode: outputs are {output_name}.bin and {output_name}.cpp")
        else:
            # Standard mode: output .bin, .cpp (loader), and .exe
            loader_path = self.generate_loader(shellcode_array, output_name, output_dir)
            loader_exe = self.compile_loader(loader_path, output_name, output_dir)
            print(f"[+] Loader executable ready at {loader_exe}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Build shellcode payloads from C++ sources")
    parser.add_argument("--inline", action="store_true", help="Enable inline hook mode")
    args = parser.parse_args()

    builder = ShellcodeBuilder(inline_hook_mode=args.inline)
    output_name = input("Enter the output file name (e.g., \"payload\"): ")
    builder.build(output_name)