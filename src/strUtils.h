#pragma once
#include "commons.h"

FUNC size_t MyStrLen(const char* str);
FUNC char* MyStrNCpy(char* dest, const char* src, size_t n);
FUNC int MyStrICmp(const char* s1, const char* s2);
FUNC int MyStrCmp(const char* s1, const char* s2);
FUNC int MyWcsICmp(const wchar_t* s1, const wchar_t* s2);
FUNC const wchar_t* GetFilenameW(const wchar_t* path);
FUNC char* MyStrChr(const char* str, int c);