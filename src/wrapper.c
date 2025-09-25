#include "wrapper.h"

// ---- Helper string and wstring comparison ----
FUNC int MyStrCmp(const char* s1, const char* s2)
{
    if (!s1 || !s2) return (s1 == s2) ? 0 : (s1 ? 1 : -1);
    while (*s1 && *s2) {
        if ((unsigned char)*s1 != (unsigned char)*s2)
            return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
        s1++; s2++;
    }
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
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

// ---- Helper: Get filename from wide path ----
FUNC const wchar_t* GetFilenameW(const wchar_t* path)
{
    if (!path) return path;
    const wchar_t* last = path;
    for (const wchar_t* p = path; *p; ++p)
        if (*p == L'\\' || *p == L'/')
            last = p + 1;
    return last;
}

// ---- Generic GetModuleBase (find loaded DLL by name, case-insensitive, returns NULL if not found) ----
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

// ---- Manual Export Resolver ----
FUNC void* GetExportByName(void* moduleBase, const char* funcName)
{
	if (!moduleBase || !funcName) return NULL;

	BYTE* base = (BYTE*)moduleBase;

	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) return NULL;

	IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE) return NULL;

	DWORD exportDirRVA = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	if (!exportDirRVA) return NULL;

	IMAGE_EXPORT_DIRECTORY* exp = (IMAGE_EXPORT_DIRECTORY*)(base + exportDirRVA);
	DWORD* names = (DWORD*)(base + exp->AddressOfNames);
	WORD* ordinals = (WORD*)(base + exp->AddressOfNameOrdinals);
	DWORD* functions = (DWORD*)(base + exp->AddressOfFunctions);

	for (DWORD i = 0; i < exp->NumberOfNames; ++i)
	{
		const char* currName = (const char*)(base + names[i]);
		if (MyStrCmp(currName, funcName) == 0)
		{
			WORD ordinal = ordinals[i];
			DWORD funcRVA = functions[ordinal];
			return (void*)(base + funcRVA);
		}
	}
	return NULL;
}

// ---- Dynamic Resolver with X-macro supporting per-function DLL search and LoadLibraryA fallback ----
FUNC int ResolveDynamicFunctions(DYNAMIC_FUNCTIONS* functions)
{
	if (!functions) return 1;

	// Always resolve kernel32 first for GetProcAddress/LoadLibraryA
	void* kernel32Base = GetModuleBase(L"kernel32.dll");
	if (!kernel32Base) return 1;

	functions->GetProcAddress = (FARPROC(WINAPI*)(HMODULE, LPCSTR))GetExportByName(kernel32Base, "GetProcAddress");
	functions->LoadLibraryA = (HMODULE(WINAPI*)(LPCSTR))GetExportByName(kernel32Base, "LoadLibraryA");

	if (!functions->GetProcAddress || !functions->LoadLibraryA)
		return 1;

#define X(ret, name, dll, ...) \
    if (MyStrCmp(#dll, "kernel32.dll") == 0) { \
        functions->name = (ret (WINAPI *)(__VA_ARGS__)) functions->GetProcAddress((HMODULE)kernel32Base, #name); \
    } else { \
        void* modBase = GetModuleBase(L##dll); \
        if (!modBase && functions->LoadLibraryA) \
            modBase = functions->LoadLibraryA(dll); \
        if (modBase && functions->GetProcAddress) \
            functions->name = (ret (WINAPI *)(__VA_ARGS__)) functions->GetProcAddress((HMODULE)modBase, #name); \
        else \
            functions->name = NULL; \
    } \
    if (!functions->name) return 1;
	WIN32_FUNC_ARSENAL
#undef X

	return 0;
}

// --- VEH: Jump to R12 (end-of-main label) on exception ---
FUNC LONG WINAPI GeneralExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
	if (pExceptionInfo->ContextRecord->R12)
	{
		pExceptionInfo->ContextRecord->Rip = pExceptionInfo->ContextRecord->R12;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

extern FUNC void Start(DYNAMIC_FUNCTIONS* funcs);

// --- Entry point wrapper ---
void StartWrapper()
{
	ALIGN_STACK();

	void* exit_label = && shellcode_exit;
	__asm__ __volatile__("mov %0, %%r12" : : "r"(exit_label));

	DYNAMIC_FUNCTIONS functions = { 0 };
	if (ResolveDynamicFunctions(&functions) != 0)
		return;

	PVOID vehHandle = functions.AddVectoredExceptionHandler(1, GeneralExceptionHandler);
	if (!vehHandle) return;

	Start(&functions);

	if (vehHandle && functions.RemoveVectoredExceptionHandler)
		functions.RemoveVectoredExceptionHandler(vehHandle);

shellcode_exit:
}