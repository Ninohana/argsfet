#ifdef EXPORTS
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif

#include <windows.h>

extern "C" API PWSTR GetCommandLineByProcessId(DWORD processId);

extern "C" API DWORD GetProcessIdByName(const wchar_t* processName);