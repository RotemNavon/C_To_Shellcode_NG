#include "commons.h"
#include "strUtils.h"

FUNC void* MyGetModuleHandle(const wchar_t* dllName)
{
    if (!dllName) return nullptr;

    PPEB peb = nullptr;
    __asm__ __volatile__("movq %%gs:0x60, %0" : "=r" (peb));
    if (!peb) return nullptr;

    PPEB_LDR_DATA ldr = peb->Ldr;
    if (!ldr) return nullptr;

    PLIST_ENTRY head = &ldr->InMemoryOrderModuleList;
    if (!head) return nullptr;

    PLIST_ENTRY current = head->Flink;
    int sanity = 0;
    while (current && current != head && sanity++ < 64)
    {
        PLDR_DATA_TABLE_ENTRY dll = CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        if (dll && dll->FullDllName.Buffer)
        {
            const wchar_t* filename = GetFilenameW(dll->FullDllName.Buffer);
            if (filename && MyWcsICmp(filename, dllName) == 0)
                return dll->DllBase;
        }
        current = current->Flink;
    }

    return nullptr;
}

FUNC bool ParseExportForwarderString(const char* forwarderString, char* outDllName, size_t dllNameCapacity, char* outExportName, size_t exportNameCapacity) {
    if (!forwarderString) return false;

    const char* period = MyStrChr(forwarderString, '.');
    if (!period) return false;

    size_t dllPartLen = (size_t)(period - forwarderString);
    if (dllPartLen >= dllNameCapacity) dllPartLen = dllNameCapacity - 1;
    MyStrNCpy(outDllName, forwarderString, dllPartLen);
    outDllName[dllPartLen] = '\0';

    size_t actualDllLen = MyStrLen(outDllName);
    if (actualDllLen < 4 || MyStrICmp(outDllName + actualDllLen - 4, ".dll") != 0) {
        MyStrNCpy(outDllName + actualDllLen, ".dll", dllNameCapacity - actualDllLen - 1);
    }

    MyStrNCpy(outExportName, period + 1, exportNameCapacity - 1);
    outExportName[exportNameCapacity - 1] = '\0';

    return true;
}

FUNC void* MyGetProcAddress(DYNAMIC_FUNCTIONS* g_functions, void* moduleBase, const char* exportName, int forwarderDepth = 0)
{
    if (!moduleBase || !exportName || forwarderDepth > 8) return nullptr;

    BYTE* moduleBytes = (BYTE*)moduleBase;

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)moduleBytes;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;

    IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(moduleBytes + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return nullptr;

    if (ntHeaders->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXPORT)
        return nullptr;

    DWORD exportDirectoryRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!exportDirectoryRVA) return nullptr;

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
                    return nullptr;

                wchar_t forwarderDllW[64];
                const char* src = forwarderDll;
                wchar_t* dst = forwarderDllW;
                while (*src && dst < forwarderDllW + 63) *dst++ = (wchar_t)(unsigned char)*src++;
                *dst = L'\0';

                void* forwarderModuleBase = MyGetModuleHandle(forwarderDllW);
                if (!forwarderModuleBase && g_functions->LoadLibraryA)
                    forwarderModuleBase = g_functions->LoadLibraryA(forwarderDll);
                if (!forwarderModuleBase)
                    return nullptr;

                return MyGetProcAddress(g_functions, forwarderModuleBase, forwarderExport, forwarderDepth + 1);
            }

            return (void*)(moduleBytes + exportAddressRVA);
        }
    }

    return nullptr;
}

FUNC int ResolveDynamicFunctions(DYNAMIC_FUNCTIONS* functions)
{
    if (!functions) return 1;

    struct FUNC_META {
        const char* name;
        const char* dll;
        void** ptr;
    };

    // First resolve LoadLibraryA manually.
    wchar_t kernel32W[] = L"kernel32.dll";
    void* kernel32Base = MyGetModuleHandle(kernel32W);
    if (!kernel32Base) return 1;

    functions->LoadLibraryA = (decltype(functions->LoadLibraryA))MyGetProcAddress(functions, kernel32Base, "LoadLibraryA");
    if (!functions->LoadLibraryA)
        return 1;

    // Define the meta array for resolution
    #define X(name, dll) { #name, dll, (void**)&functions->name },
    FUNC_META meta[] = {
        WIN32_FUNC_ARSENAL
    };
    #undef X

    // Loop through the rest of the APIs starting from index 1
    for (size_t i = 1; i < sizeof(meta)/sizeof(meta[0]); ++i) {
        FUNC_META& entry = meta[i];

        // Convert DLL name to wchar_t
        wchar_t wDll[64];
        const char* src = entry.dll;
        wchar_t* dst = wDll;
        while (*src && dst < wDll + 63) *dst++ = (wchar_t)(unsigned char)*src++;
        *dst = L'\0';

        // Get the module base
        void* modBase = MyGetModuleHandle(wDll);

        // If not loaded, use LoadLibraryA
        if (!modBase && functions->LoadLibraryA)
            modBase = ((void*(*)(const char*))functions->LoadLibraryA)(entry.dll);

        // Resolve export address
        void* addr = modBase ? MyGetProcAddress(functions, modBase, entry.name, 0) : nullptr;

        // Store the resolved address in DYNAMIC_FUNCTIONS
        *(entry.ptr) = addr;

        if (!addr)
        {
            return 1; // error: could not resolve function
        }
    }

    return 0;
}

extern FUNC void Main(DYNAMIC_FUNCTIONS* g_functions);

FUNC void StartWrapperImpl()
{
    DYNAMIC_FUNCTIONS g_functions = {};
    if (ResolveDynamicFunctions(&g_functions) != 0) return;

    Main(&g_functions);
}

#ifdef INLINE_HOOK_MODE
    #define STACK_ALLOC_SIZE  0x38
    #define WRAPPER_EXIT      "nop; ud2; ud2;"
#else
    #define STACK_ALLOC_SIZE  0x40
    #define WRAPPER_EXIT      "ret;"
#endif

__attribute__((naked, optimize("O0"))) void StartWrapper()
{
    __asm__ __volatile__(
        "push %%rax; push %%rcx; push %%rdx; push %%rbx; push %%rsi; push %%rdi; push %%rbp; "
        "push %%r8; push %%r9; push %%r10; push %%r11; push %%r12; push %%r13; push %%r14; push %%r15;"
        "sub %0, %%rsp;"
        :
        : "i" (STACK_ALLOC_SIZE)
        : "memory"
    );

    StartWrapperImpl();

    __asm__ __volatile__(
        "add %0, %%rsp;"
        "pop %%r15; pop %%r14; pop %%r13; pop %%r12; pop %%r11; pop %%r10; pop %%r9; pop %%r8; "
        "pop %%rbp; pop %%rdi; pop %%rsi; pop %%rbx; pop %%rdx; pop %%rcx; pop %%rax;"
        WRAPPER_EXIT
        :
        : "i" (STACK_ALLOC_SIZE)
        : "memory"
    );
}