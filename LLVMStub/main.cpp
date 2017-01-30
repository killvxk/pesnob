#include <stdio.h>
#include <Windows.h>
#include "typedefs.h"
#include "stub.h"
#include "macros.h"

PIMAGE_SECTION_HEADER get_stub_binary(char* filePath);
PIMAGE_SECTION_HEADER find_stub(PIMAGE_DOS_HEADER dos, PIMAGE_NT_HEADERS nt, int magic);
DWORD get_original_entrypoint();


DWORD get_original_entrypoint()
{
	HMODULE module = GetModuleHandle(NULL);
	PIMAGE_DOS_HEADER pImageDosHeader = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS32 pImageNtHeaders = CALC_OFFSET(PIMAGE_NT_HEADERS32, pImageDosHeader, pImageDosHeader->e_lfanew);
	return pImageNtHeaders->OptionalHeader.AddressOfEntryPoint;
}


int main(int argc, char** argv)
{
	DWORD oep = get_original_entrypoint();
	printf("oldEP: %.08X\n", oep);

	getchar();
	return 0;
}