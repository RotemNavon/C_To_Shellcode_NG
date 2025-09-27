#include "wrapper.h"
#include <lmcons.h>   // for UNLEN

// --- Actual logic entry ---
FUNC void Start()
{
    HANDLE hStdOut = g_functions.GetStdHandle(STD_OUTPUT_HANDLE);
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;

    if (g_functions.GetUserNameA(username, &username_len)) {
        g_functions.WriteFile(hStdOut, username, username_len, nullptr, nullptr);
    } 
    else 
    {
        char errBuffer[] = "Error getting username";
        g_functions.WriteFile(hStdOut, errBuffer, sizeof(errBuffer), nullptr, nullptr);
    }
}