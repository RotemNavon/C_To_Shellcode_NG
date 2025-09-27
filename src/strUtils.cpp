#include "strUtils.h"

FUNC size_t MyStrlen(const char* str)
{
    size_t len = 0;
    while (str && str[len])
        ++len;
    return len;
}

FUNC char* MyStrncpy(char* dest, const char* src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i]; ++i)
        dest[i] = src[i];
    for (; i < n; ++i)
        dest[i] = '\0';
    return dest;
}

FUNC int MyStricmp(const char* s1, const char* s2)
{
    if (!s1 || !s2) return (s1 == s2) ? 0 : (s1 ? 1 : -1);
    while (*s1 && *s2) {
        unsigned char c1 = (unsigned char)*s1;
        unsigned char c2 = (unsigned char)*s2;
        if (c1 >= 'A' && c1 <= 'Z') c1 += 0x20;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 0x20;
        if (c1 != c2) return (int)c1 - (int)c2;
        ++s1; ++s2;
    }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}

FUNC int MyStrCmp(const char* s1, const char* s2)
{
    if (!s1 || !s2) return (s1 == s2) ? 0 : (s1 ? 1 : -1);
    while (*s1 && *s2) {
        if ((unsigned char)*s1 != (unsigned char)*s2)
            return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
        s1++; s2++;
    }
    return (int)(unsigned char)*s1 - (unsigned char)*s2;
}

FUNC int MyWcsIcmp(const wchar_t* s1, const wchar_t* s2)
{
    if (!s1 || !s2) return (s1 == s2) ? 0 : (s1 ? 1 : -1);
    while (*s1 && *s2) {
        wchar_t c1 = *s1, c2 = *s2;
        if (c1 >= L'A' && c1 <= L'Z') c1 += 0x20;
        if (c2 >= L'A' && c2 <= L'Z') c2 += 0x20;
        if (c1 != c2) return (int)c1 - (int)c2;
        s1++; s2++;
    }
    return (int)(*s1 - *s2);
}

FUNC const wchar_t* GetFilenameW(const wchar_t* path)
{
    if (!path) return path;
    const wchar_t* last = path;
    for (const wchar_t* p = path; *p; ++p)
        if (*p == L'\\' || *p == L'/')
            last = p + 1;
    return last;
}

FUNC char* MyStrchr(const char* str, int c)
{
    if (!str) return nullptr;
    while (*str) {
        if (*str == (char)c)
            return (char*)str;
        ++str;
    }
    return (c == 0) ? (char*)str : nullptr;
}