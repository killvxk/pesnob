#include <stdio.h>
#include <Windows.h>

#include <vector>
#include <list>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "typedefs.h"
#include "stub.h"
#include "helper.h"
#include "pe_lib\pe_bliss.h"

using namespace std;
using namespace pe_bliss;

PIMAGE_SECTION_HEADER get_stub_binary(char* filePath);
PIMAGE_SECTION_HEADER find_stub(PIMAGE_DOS_HEADER dos, PIMAGE_NT_HEADERS nt, int magic);
DWORD get_original_entrypoint();
DWORD get_stub_entrypoint_offset();

DWORD get_original_entrypoint()
{
	HMODULE module = GetModuleHandle(NULL);
	PIMAGE_DOS_HEADER pImageDosHeader = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS32 pImageNtHeaders = CALC_OFFSET(PIMAGE_NT_HEADERS32, pImageDosHeader, pImageDosHeader->e_lfanew);
	return pImageNtHeaders->OptionalHeader.AddressOfEntryPoint;
}

// Copy the stub sections, order matters
char* create_stub_blob(const char** section_names, size_t num_sections, size_t *out_size)
{
	if (num_sections == 0)
		return 0;
	HMODULE thisMod = GetModuleHandle(nullptr);
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)thisMod;
	PIMAGE_NT_HEADERS32 nt = CALC_OFFSET(PIMAGE_NT_HEADERS32, dos, dos->e_lfanew);
	short numSections = nt->FileHeader.NumberOfSections;
	//naive approach, but w.e
	if (numSections < num_sections)
	{
		cout << "Where the hell is our stub" << endl;
		return 0;
	}
	string stub_data;
	stringstream ss;
	PIMAGE_SECTION_HEADER sec = IMAGE_FIRST_SECTION(nt);
	for (int i = 0; i < numSections; ++i)
	{
		for (int j = 0; j < num_sections; j++)
		{
			if (strcmp(section_names[j], (char*)sec[i].Name) == 0)
			{
				cout << "Section Address: "<< hex << (DWORD)thisMod + sec[i].VirtualAddress << endl;
				char *data = (char*)(DWORD)thisMod + sec[i].VirtualAddress;
				//ensure we have the space
				stub_data.resize(sec[i].SizeOfRawData);
				memcpy(&stub_data[0], data, sec[i].SizeOfRawData);
				//trim the section
				strip_nullbytes(stub_data);
				ss << stub_data;
				stub_data.clear();
			}
		}
		
	}
	cout << "Total Stub Size: " << ss.str().size() << endl;
	char* ret = (char*)malloc(ss.str().size());
	*out_size = ss.str().size();
	memcpy(ret, const_cast<char*>(ss.str().data()), ss.str().size());
	return ret;
}

int main(int argc, char** argv)
{
	//Stub sections we want to copy
	const char *stubs [] = {
		".stub",
		".crt"
	};

	//extract stub data from packer body
	size_t stub_size;
	char *ret = create_stub_blob(stubs, 2, &stub_size);
	if (stub_size == 0)
	{
		cout << "Couldn't find stub data!" << endl;
		return 0;
	}
	string new_stub(ret,stub_size);
	free(ret);

	config stub1{
		0
	};

	results *out = (results*)malloc(sizeof(results));
	memset(out, 0, sizeof(results));

	char test_file[] = { "C:\\git_code\\stubber\\Release\\SmallExe.exe" };

	ifstream target(test_file, std::ios::in | std::ios::binary);
	if (!target)
	{
		cout << "Error Reading File" << endl;
		return 0;
	}

	try 
	{
		// parse the pe file
		cout << "Parsing PE File" << endl;
		pe_base image(pe_factory::create_pe(target));
		// dotnet not supported
		if (image.is_dotnet())
		{
			cout << "Unsupported File" << endl;
			return 0;
		}
		//create the packer section
		section new_section;
		new_section.readable(true).writeable(true);
		new_section.set_name("glpack");
		new_section.set_raw_data(new_stub);
		new_section.set_size_of_raw_data(align_up(new_stub.size(), image.get_section_alignment()));

		section &added_section = image.add_section(new_section);
		image.set_section_virtual_size(added_section, 0x1000);

		//output new file
		std::string base_file_name(test_file);
		std::string::size_type slash_pos;
		if ((slash_pos = base_file_name.find_last_of("/\\")) != std::string::npos)
			base_file_name = base_file_name.substr(slash_pos + 1);

		base_file_name = "new_" + base_file_name;
		std::ofstream new_pe_file(base_file_name.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
		if (!new_pe_file)
		{
			std::cout << "Cannot create " << base_file_name << std::endl;
			return -1;
		}
		cout << "Rebuilding PE file with new section" << endl;
		rebuild_pe(image, new_pe_file);
	}
	catch (const pe_exception &e)
	{
		cout << "Exception: "<< e.what() << endl;
	}
	/*
	stub_entry entry = (stub_entry)entrypoint;
	if (entry)
	{
		entry((void*)&stub1, (void*)out);
	}
	if (!out->continuable)
	{
		cout << "Stub returned fail: " << out->res_value << endl;
	}
	*/
	getchar();
	return 0;
}