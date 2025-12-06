#pragma once
#include <windows.h>
#include <winternl.h>
#include <stdio.h>

#define FUNC __attribute__((section(".func")))

// ---- X-Macro Arsenal ----
#define WIN32_FUNC_ARSENAL \
    X(LoadLibraryA,                     "kernel32.dll") \
    X(ShellExecuteA,                    "shell32.dll")

// ---- Generate function pointer struct, using decltype
struct DYNAMIC_FUNCTIONS
{
#define X(name, dll) decltype(&name) name = nullptr;
    WIN32_FUNC_ARSENAL
#undef X
};