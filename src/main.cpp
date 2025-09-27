#include "wrapper.h"

GLOBAL_VAR int my_global_counter;

FUNC void UpdateGlobal()
{
    my_global_counter = 47;
}

FUNC void GlobalVarTest()
{
    char buf[32] = {0};
    
    my_global_counter = 0;
    g_functions.sprintf(buf, "Value: %d", my_global_counter);
    g_functions.MessageBoxA(NULL, buf, "Global Value", MB_OK);

    UpdateGlobal();
    g_functions.sprintf(buf, "Value: %d", my_global_counter);
    g_functions.MessageBoxA(NULL, buf, "Global Value After Update", MB_OK);
}

// --- Actual shellcode entry ---
FUNC void Start()
{    
    // Create a local pointer to force direct structure access
    g_functions.ShellExecuteA(NULL, "open", "notepad.exe", NULL, NULL, SW_SHOW);

    GlobalVarTest();
}