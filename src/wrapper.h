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
// return type, function name, source dll, WinAPI prototype
#define WIN32_FUNC_ARSENAL \
    X(FARPROC, GetProcAddress,             "kernel32.dll", HMODULE, LPCSTR) \
    X(HMODULE, LoadLibraryA,               "kernel32.dll", LPCSTR) \
    X(PVOID, AddVectoredExceptionHandler,  "kernel32.dll", ULONG, PVECTORED_EXCEPTION_HANDLER) \
    X(ULONG, RemoveVectoredExceptionHandler,"kernel32.dll", PVOID) \
    X(HINSTANCE,  ShellExecuteA,           "shell32.dll", HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT)

// ---- Generate typedefs and struct ----
typedef struct _DYNAMIC_FUNCTIONS
{
#define X(ret, name, dll, ...) ret (WINAPI *name)(__VA_ARGS__);
    WIN32_FUNC_ARSENAL
#undef X
} DYNAMIC_FUNCTIONS;

// ----tion prototypes ----
FUNC int MyStrCmp(const char* s1, const char* s2);
FUNC int MyWcsIcmp(const wchar_t* s1, const wchar_t* s2);
FUNC const wchar_t* GetFilenameW(const wchar_t* path);
FUNC void* GetModuleBase(const wchar_t* dllName);
FUNC void* GetExportByName(void* moduleBase, const char*Name);
FUNC int ResolveDynamicFunctions(DYNAMIC_FUNCTIONS*s);
FUNC LONG WINAPI GeneralExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);
void StartWrapper();