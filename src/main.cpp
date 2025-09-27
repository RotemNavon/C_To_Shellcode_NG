#include "wrapper.h"

GLOBAL_VAR int my_global_counter;

FUNC void GlobalVarTest()
{
    char buf[32] = {0};
    
    my_global_counter = 0;
    g_functions.sprintf(buf, "Value: %d", my_global_counter);
    g_functions.MessageBoxA(nullptr, buf, "Global Value", MB_OK);

    my_global_counter = 47;
    g_functions.sprintf(buf, "Value: %d", my_global_counter);
    g_functions.MessageBoxA(nullptr, buf, "Global Value After Update", MB_OK);
}

// --- Actual logic entry ---
FUNC void Start()
{
    g_functions.ShellExecuteA(nullptr, "open", "notepad.exe", nullptr, nullptr, SW_SHOW);

    GlobalVarTest();
}