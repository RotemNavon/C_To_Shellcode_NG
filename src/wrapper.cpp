#include "wrapper.h"
#include "strUtils.h"

FUNC void* GetModuleBase(const wchar_t* dllName)
{
    if (!dllName) return NULL;

    PPEB peb = NULL;
    __asm__ __volatile__("movq %%gs:0x60, %0" : "=r" (peb));
    if (!peb) return NULL;

    PPEB_LDR_DATA ldr = peb->Ldr;
    if (!ldr) return NULL;

    PLIST_ENTRY head = &ldr->InMemoryOrderModuleList;
    if (!head) return NULL;

    PLIST_ENTRY current = head->Flink;
    int sanity = 0;
    while (current && current != head && sanity++ < 64)
    {
        PLDR_DATA_TABLE_ENTRY dll = CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        if (dll && dll->FullDllName.Buffer)
        {
            const wchar_t* filename = GetFilenameW(dll->FullDllName.Buffer);
            if (filename && MyWcsIcmp(filename, dllName) == 0)
                return dll->DllBase;
        }
        current = current->Flink;
    }
    return NULL;
}

FUNC bool ParseExportForwarderString(const char* forwarderString, char* outDllName, size_t dllNameCapacity, char* outExportName, size_t exportNameCapacity) {
    if (!forwarderString) return false;
    const char* period = MyStrchr(forwarderString, '.');
    if (!period) return false;
    size_t dllPartLen = (size_t)(period - forwarderString);
    if (dllPartLen >= dllNameCapacity) dllPartLen = dllNameCapacity - 1;
    MyStrncpy(outDllName, forwarderString, dllPartLen);
    outDllName[dllPartLen] = '\0';

    size_t actualDllLen = MyStrlen(outDllName);
    if (actualDllLen < 4 || MyStricmp(outDllName + actualDllLen - 4, ".dll") != 0) {
        MyStrncpy(outDllName + actualDllLen, ".dll", dllNameCapacity - actualDllLen - 1);
    }
    MyStrncpy(outExportName, period + 1, exportNameCapacity - 1);
    outExportName[exportNameCapacity - 1] = '\0';
    return true;
}

FUNC void* ResolveExportByName(void* moduleBase, const char* exportName, int forwarderDepth = 0, void* (*LoadLibraryA)(const char*) = nullptr)
{
    if (!moduleBase || !exportName || forwarderDepth > MAX_FORWARDER_RECURSION_DEPTH) return NULL;

    BYTE* moduleBytes = (BYTE*)moduleBase;
    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)moduleBytes;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return NULL;
    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(moduleBytes + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return NULL;

    if (ntHeaders->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT)
        return NULL;

    DWORD exportDirectoryRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!exportDirectoryRVA) return NULL;

    IMAGE_EXPORT_DIRECTORY* exportDirectory = (IMAGE_EXPORT_DIRECTORY*)(moduleBytes + exportDirectoryRVA);
    DWORD* exportNameRVAs = (DWORD*)(moduleBytes + exportDirectory->AddressOfNames);
    WORD* exportOrdinals = (WORD*)(moduleBytes + exportDirectory->AddressOfNameOrdinals);
    DWORD* exportFunctionRVAs = (DWORD*)(moduleBytes + exportDirectory->AddressOfFunctions);

    for (DWORD i = 0; i < exportDirectory->NumberOfNames; ++i)
    {
        const char* currentExportName = (const char*)(moduleBytes + exportNameRVAs[i]);
        if (MyStrCmp(currentExportName, exportName) == 0)
        {
            WORD ordinal = exportOrdinals[i];
            DWORD exportAddressRVA = exportFunctionRVAs[ordinal];

            DWORD exportDirectoryStart = exportDirectoryRVA;
            DWORD exportDirectoryEnd = exportDirectoryStart + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
            if (exportAddressRVA >= exportDirectoryStart && exportAddressRVA < exportDirectoryEnd)
            {
                const char* forwarderString = (const char*)(moduleBytes + exportAddressRVA);
                char forwarderDll[64], forwarderExport[64];
                if (!ParseExportForwarderString(forwarderString, forwarderDll, sizeof(forwarderDll), forwarderExport, sizeof(forwarderExport)))
                    return NULL;

                wchar_t forwarderDllW[64];
                const char* src = forwarderDll;
                wchar_t* dst = forwarderDllW;
                while (*src && dst < forwarderDllW + 63) *dst++ = (wchar_t)(unsigned char)*src++;
                *dst = L'\0';

                void* forwarderModuleBase = GetModuleBase(forwarderDllW);
                if (!forwarderModuleBase && LoadLibraryA)
                    forwarderModuleBase = LoadLibraryA(forwarderDll);
                if (!forwarderModuleBase)
                    return NULL;

                return ResolveExportByName(forwarderModuleBase, forwarderExport, forwarderDepth + 1, LoadLibraryA);
            }
            return (void*)(moduleBytes + exportAddressRVA);
        }
    }
    return NULL;
}

FUNC int ResolveDynamicFunctions(DYNAMIC_FUNCTIONS* functions)
{
    if (!functions) return 1;

    wchar_t kernel32W[] = L"kernel32.dll";
    void* kernel32Base = GetModuleBase(kernel32W);
    if (!kernel32Base) return 1;

    functions->LoadLibraryA = (decltype(&LoadLibraryA))ResolveExportByName(kernel32Base, "LoadLibraryA");
    if (!functions->LoadLibraryA)
        return 1;

#define X(name, dll) \
    { \
        wchar_t wDll[64]; \
        const char* src = dll; \
        wchar_t* dst = wDll; \
        while (*src && dst < wDll + 63) *dst++ = (wchar_t)(unsigned char)*src++; \
        *dst = L'\0'; \
        void* modBase = GetModuleBase(wDll); \
        if (!modBase && functions->LoadLibraryA) \
            modBase = functions->LoadLibraryA(dll); \
        auto loader = (void* (*)(const char*))functions->LoadLibraryA; \
        functions->name = modBase ? (decltype(&name))ResolveExportByName(modBase, #name, 0, loader) : nullptr; \
        if (!functions->name) return 1; \
    }
    WIN32_FUNC_ARSENAL
#undef X

    return 0;
}

FUNC LONG WINAPI GeneralExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
    __debugbreak();
    CONTEXT* ctx = pExceptionInfo ? pExceptionInfo->ContextRecord : nullptr;
    if (ctx && ctx->R12 && ctx->Rip != ctx->R12) {
        ctx->Rip = ctx->R12;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void StartWrapper()
{
    ALIGN_STACK();

    // save the exit label to a register for execution continuation after exception
    void* exit_label = &&shellcode_exit;
    __asm__ __volatile__("mov %0, %%r12" : : "r"(exit_label));

    // resolve all api functions
    DYNAMIC_FUNCTIONS functions = {};
    if (ResolveDynamicFunctions(&functions) != 0)
        return;

    // add the vectored exception handler
    PVOID vehHandle = functions.AddVectoredExceptionHandler ?
        functions.AddVectoredExceptionHandler(1, GeneralExceptionHandler) : NULL;
    if (!vehHandle) return;

    Start(&functions);

shellcode_exit:

    // remove the handler
    if (vehHandle && functions.RemoveVectoredExceptionHandler)
        functions.RemoveVectoredExceptionHandler(vehHandle);
}