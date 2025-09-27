#pragma once
#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <stdio.h>

#define FUNC __attribute__((section(".func")))
#define NAKED __attribute__((naked))
#define NO_OPTIMIZE __attribute__((optimize("O0")))
#define GLOBAL_VAR inline

#define ALIGN_STACK() \
  __asm__ __volatile__( \
      "mov %%rsp, %%rax;" \
      "and $0xF, %%rax;" \
      "jz 1f;" \
      "sub $8, %%rsp;" \
      "1:" \
      : : : "%rax" \
  )

// ---- X-Macro Arsenal ----
#define WIN32_FUNC_ARSENAL \
    X(LoadLibraryA,                     "kernel32.dll") \
    X(AddVectoredExceptionHandler,      "kernel32.dll") \
    X(RemoveVectoredExceptionHandler,   "kernel32.dll") \
    X(CloseHandle,                      "kernel32.dll") \
    X(CreateToolhelp32Snapshot,         "kernel32.dll") \
    X(Process32First,                   "kernel32.dll") \
    X(Process32Next,                    "kernel32.dll") \
    X(GetProcessHeap,                   "kernel32.dll") \
    X(HeapAlloc,                        "kernel32.dll") \
    X(HeapReAlloc,                      "kernel32.dll") \
    X(HeapFree,                         "kernel32.dll") \
    X(_snprintf,                        "msvcrt.dll")   \
    X(GetStdHandle,                     "kernel32.dll") \
    X(WriteFile,                        "kernel32.dll")

// ---- Generate function pointer struct, using decltype
struct DYNAMIC_FUNCTIONS
{
#define X(name, dll) decltype(&name) name = nullptr;
    WIN32_FUNC_ARSENAL
#undef X
};

// Global functions structure instance - defined here to avoid extern linkage issues
GLOBAL_VAR DYNAMIC_FUNCTIONS g_functions = {};

// Global exit address for VEH handler
GLOBAL_VAR void* g_exit_address = nullptr;

// Global VEH handle for cleanup
GLOBAL_VAR void* g_veh_handle = nullptr;