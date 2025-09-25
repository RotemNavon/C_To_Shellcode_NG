#pragma once
#include <windows.h>
#include <winternl.h>

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

// ---- X-Macro Arsenal ----
#define WIN32_FUNC_ARSENAL \
    X(GetProcAddress,                   "kernel32.dll") \
    X(LoadLibraryA,                     "kernel32.dll") \
    X(AddVectoredExceptionHandler,      "kernel32.dll") \
    X(RemoveVectoredExceptionHandler,   "kernel32.dll") \
    X(ShellExecuteA,                    "shell32.dll")

// ---- Generate function pointer struct, using decltype
struct DYNAMIC_FUNCTIONS
{
#define X(name, dll) decltype(&name) name = nullptr;
    WIN32_FUNC_ARSENAL
#undef X
};

// ---- Function prototypes ----
FUNC int MyStrCmp(const char* s1, const char* s2);
FUNC int MyWcsIcmp(const wchar_t* s1, const wchar_t* s2);
FUNC const wchar_t* GetFilenameW(const wchar_t* path);
FUNC void* GetModuleBase(const wchar_t* dllName);
FUNC void* GetExportByName(void* moduleBase, const char*Name);
FUNC int ResolveDynamicFunctions(DYNAMIC_FUNCTIONS* s);
FUNC LONG WINAPI GeneralExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);
void StartWrapper();
FUNC void Start(DYNAMIC_FUNCTIONS* funcs);