#pragma once
#include "commons.h"

FUNC size_t MyStrlen(const char* str);
FUNC char* MyStrncpy(char* dest, const char* src, size_t n);
FUNC int MyStricmp(const char* s1, const char* s2);
FUNC int MyStrCmp(const char* s1, const char* s2);
FUNC int MyWcsIcmp(const wchar_t* s1, const wchar_t* s2);
FUNC const wchar_t* GetFilenameW(const wchar_t* path);
FUNC char* MyStrchr(const char* str, int c);