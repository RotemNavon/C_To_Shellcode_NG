#pragma once
#include "commons.h"

#define MAX_FORWARDER_RECURSION_DEPTH 8

FUNC void* MyGetModuleHandle(const wchar_t* dllName);
FUNC bool ParseExportForwarderString(const char* forwarderString, char* outDllName, size_t dllNameCapacity, char* outExportName, size_t exportNameCapacity);
FUNC void* MyGetProcAddress(void* moduleBase, const char* exportName, int forwarderDepth);
FUNC int ResolveDynamicFunctions(DYNAMIC_FUNCTIONS* functions);
FUNC void RemoveVEHHandler();
FUNC long WINAPI GeneralExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);

void StartWrapper();