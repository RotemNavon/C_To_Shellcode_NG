#include "wrapper.h"

// --- Actual shellcode entry ---
FUNC void Start(DYNAMIC_FUNCTIONS* functions)
{
    functions->ShellExecuteA(NULL, "open", "notepad.exe", NULL, NULL, SW_SHOW);
}