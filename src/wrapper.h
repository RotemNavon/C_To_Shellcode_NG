#pragma once
#include "commons.h"

#define MAX_FORWARDER_RECURSION_DEPTH 8

FUNC void* GetModuleBase(const wchar_t* dllName);
FUNC bool ParseExportForwarderString(const char* forwarderString, char* outDllName, size_t dllNameCapacity, char* outExportName, size_t exportNameCapacity);
FUNC void* GetExportByName(void* moduleBase, const char*Name);
FUNC int ResolveDynamicFunctions(DYNAMIC_FUNCTIONS* functions);
FUNC LONG WINAPI GeneralExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);
void StartWrapper();
FUNC void Start(DYNAMIC_FUNCTIONS* functions);