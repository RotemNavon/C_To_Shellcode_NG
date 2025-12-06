#include "commons.h"

FUNC void Main(DYNAMIC_FUNCTIONS* g_functions)
{
    g_functions->ShellExecuteA(nullptr, "open", "notepad.exe", nullptr, nullptr, SW_SHOW);
}