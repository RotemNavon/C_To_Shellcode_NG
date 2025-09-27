#include "wrapper.h"

int my_global_counter;

FUNC void UpdateGlobal()
{
    my_global_counter = 47;
}

FUNC void GlobalTest(DYNAMIC_FUNCTIONS* functions)
{
    char buf[32] = {0};
    
    my_global_counter = 0;
    functions->sprintf(buf, "Value: %d", my_global_counter);
    functions->MessageBoxA(NULL, buf, "Global Value", MB_OK);

    UpdateGlobal();
    functions->sprintf(buf, "Value: %d", my_global_counter);
    functions->MessageBoxA(NULL, buf, "Global Value After Update", MB_OK);
}

// --- Actual shellcode entry ---
FUNC void Start(DYNAMIC_FUNCTIONS* functions)
{
    functions->ShellExecuteA(NULL, "open", "notepad.exe", NULL, NULL, SW_SHOW);

    GlobalTest(functions);
}