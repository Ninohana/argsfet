#include "pch.h"

#include "argsfet.hpp"

#include <winternl.h>
#include <tlhelp32.h>
#include <stdio.h>

typedef NTSTATUS(WINAPI* PNTQUERYINFORMATIONPROCESS)(
	HANDLE ProcessHandle,
	ULONG ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength);

PWSTR GetCommandLineByProcessId(DWORD processId) {
	// Open process
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		printf("OpenProcess failed with error: %d\n", GetLastError());
		return nullptr;
	}

	BOOL isWow64 = FALSE;
	IsWow64Process(hProcess, &isWow64);
	if (isWow64)
	{
		printf("Process is 64-bit\n");
	}

	// Load ntdll and get NtQueryInformationProcess address
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	PNTQUERYINFORMATIONPROCESS NtQueryInformationProcess =
		(PNTQUERYINFORMATIONPROCESS)GetProcAddress(hNtDll, "NtQueryInformationProcess");
	if (NtQueryInformationProcess)
	{
		// Get process information
		PROCESS_BASIC_INFORMATION pbi;
		ULONG returnLength;
		NTSTATUS status = NtQueryInformationProcess(hProcess, 0, &pbi, sizeof(pbi), &returnLength);
		if (NT_SUCCESS(status))
		{
			// Read PEB
			PEB peb;
			if (ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), NULL))
			{
				RTL_USER_PROCESS_PARAMETERS upp;
				if (ReadProcessMemory(hProcess, peb.ProcessParameters, &upp, sizeof(upp), NULL))
				{
					WCHAR* cmdLineBuffer = new WCHAR[upp.CommandLine.MaximumLength / sizeof(WCHAR)];
					if (ReadProcessMemory(hProcess, upp.CommandLine.Buffer, cmdLineBuffer, upp.CommandLine.MaximumLength, NULL)) {
						CloseHandle(hProcess);
						return cmdLineBuffer;
					}
				}
			}
		}
		else
		{
			printf("NtQueryInformationProcess failed with error: %d\n", status);
		}

	}
	else
	{
		printf("Get NtQueryInformationProcess addres failed with error: %d\n", GetLastError());
	}

	CloseHandle(hProcess);
	return nullptr;
}

DWORD GetProcessIdByName(const wchar_t* processName)
{
	DWORD processId = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32W pe;
		pe.dwSize = sizeof(PROCESSENTRY32W);
		if (Process32FirstW(hSnapshot, &pe)) {
			do {
				if (wcscmp(pe.szExeFile, processName) == 0) {
					processId = pe.th32ProcessID;
					break;
				}
			} while (Process32NextW(hSnapshot, &pe));
		}
		CloseHandle(hSnapshot);
	}
	return processId;
}
