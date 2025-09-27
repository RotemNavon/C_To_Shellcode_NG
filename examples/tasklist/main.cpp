#include "wrapper.h"

// --- Actual logic entry ---
FUNC void Start()
{
    HANDLE hSnapshot = g_functions.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hSnapshot == INVALID_HANDLE_VALUE){
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if(!g_functions.Process32First(hSnapshot, &pe32)){
        g_functions.CloseHandle(hSnapshot);
        return;
    }

    size_t bufferSize = 4096;
    HANDLE hHeap = g_functions.GetProcessHeap();
    if(!hHeap){
        g_functions.CloseHandle(hSnapshot);
        return;
    }

    // Equivalent to calloc: HEAP_ZERO_MEMORY flag
    char* buffer = (char*)g_functions.HeapAlloc(hHeap, HEAP_ZERO_MEMORY, bufferSize);
    if(!buffer){
        g_functions.CloseHandle(hSnapshot);
        return;
    }

    size_t offset = 0;
    do
    {
        // Estimate required space for this entry
        const char format[] = "PID: %5u | Executable: %s\n";
        int required = g_functions._snprintf(nullptr, 0, format, pe32.th32ProcessID, pe32.szExeFile);
        if (required < 0) {
            g_functions.HeapFree(hHeap, 0, buffer);
            g_functions.CloseHandle(hSnapshot);
            return;
        }

        // Resize buffer if needed
        if (offset + required + 1 > bufferSize) {
            size_t newSize = bufferSize * 2;
            while (offset + required + 1 > newSize) {
                newSize *= 2;
            }
            char* newBuffer = (char*)g_functions.HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, buffer, newSize);
            if (!newBuffer) {
                g_functions.HeapFree(hHeap, 0, buffer);
                g_functions.CloseHandle(hSnapshot);
                return;
            }
            buffer = newBuffer;
            bufferSize = newSize;
        }

        // Append formatted string
        int written = g_functions._snprintf(buffer + offset, bufferSize - offset, format, pe32.th32ProcessID, pe32.szExeFile);
        if (written > 0) {
            offset += written;
        }
    } while (g_functions.Process32Next(hSnapshot, &pe32));

    g_functions.CloseHandle(hSnapshot);

    HANDLE hStdOut = g_functions.GetStdHandle(STD_OUTPUT_HANDLE);
    g_functions.WriteFile(hStdOut, buffer, bufferSize, nullptr, nullptr);

    g_functions.HeapFree(hHeap, 0, buffer);
}