#pragma once
#include <windows.h>
#include <winternl.h>
#include <stdio.h>

#define FUNC __attribute__((section(".func")))

#define ALIGN_STACK() \
  __asm__ __volatile__( \
      "mov %%rsp, %%rax;" \
      "and $0xF, %%rax;" \
      "jz 1f;" \
      "sub $8, %%rsp;" \
      "1:" \
      : : : "%rax" \
  )

#define MAX_FORWARDER_RECURSION_DEPTH 8

// ---- X-Macro Arsenal ----
#define WIN32_FUNC_ARSENAL \
    X(LoadLibraryA,                     "kernel32.dll") \
    X(AddVectoredExceptionHandler,      "kernel32.dll") \
    X(RemoveVectoredExceptionHandler,   "kernel32.dll") \
    X(ShellExecuteA,                    "Shell32.dll")  \
    X(MessageBoxA,                      "user32.dll")   \
    X(sprintf,                          "msvcrt.dll")

// ---- Generate function pointer struct, using decltype
struct DYNAMIC_FUNCTIONS
{
#define X(name, dll) decltype(&name) name = nullptr;
    WIN32_FUNC_ARSENAL
#undef X
};