#pragma once
#include "commons.h"

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

FUNC void* MyGetModuleHandle(const wchar_t* dllName);
FUNC bool ParseExportForwarderString(const char* forwarderString, char* outDllName, size_t dllNameCapacity, char* outExportName, size_t exportNameCapacity);
FUNC void* MyGetProcAddress(void* moduleBase, const char* exportName, int forwarderDepth);
FUNC int ResolveDynamicFunctions(DYNAMIC_FUNCTIONS* functions);
FUNC void RemoveVEHHandler();
FUNC long WINAPI GeneralExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);

void StartWrapper();

// Global exit address for VEH handler
GLOBAL_VAR void* g_exit_address = nullptr;

// Global VEH handle for cleanup
GLOBAL_VAR void* g_veh_handle = nullptr;