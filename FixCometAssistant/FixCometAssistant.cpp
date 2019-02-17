// FixCometAssistant.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <mhook-lib/mhook.h>
#include <TlHelp32.h>


// 注意这里的开关
// 是否修复所有进程路径
// 因为只有目标进程为 64 位时才会出问题，其实可以不用管 32 位进程
bool g_hookAllProcess = true;

#define DEFHOOKFUNCTYPE(func) \
	typedef decltype (func) fn##func; \
	auto g_pfn##func = func;

DEFHOOKFUNCTYPE(CreateToolhelp32Snapshot);
DEFHOOKFUNCTYPE(Module32FirstW);
DEFHOOKFUNCTYPE(CloseHandle);

#undef DEFHOOKFUNCTYPE

std::list<HANDLE> g_fakeHandleList;

BOOL IsX64Process(HANDLE hProcess)
{
	BOOL iswow64;
	return IsWow64Process(hProcess, &iswow64) && !iswow64;
}

HANDLE WINAPI MyCreateToolhelp32Snapshot(DWORD dwFlags, DWORD th32ProcessID)
{
	auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, th32ProcessID);
	if (hProcess)
	{
		if (g_hookAllProcess || IsX64Process(hProcess))
		{
			// 如果是 64 位进程，把目标进程句柄当作快照句柄返回
			// 返回一个真实存在的句柄，而不是虚构一个值，可以避免与已有句柄冲突
			// 并且也能无缝兼容 CloseHandle
			g_fakeHandleList.push_back(hProcess);
			return hProcess;
		}
		else
		{
			CloseHandle(hProcess);
		}
	}
	return g_pfnCreateToolhelp32Snapshot(dwFlags, th32ProcessID);
}

BOOL WINAPI MyModule32FirstW(HANDLE hSnapshot, LPMODULEENTRY32W lpme)
{
	if (std::find(g_fakeHandleList.begin(), g_fakeHandleList.end(), hSnapshot) != g_fakeHandleList.end())
	{
		DWORD pathsize = sizeof(lpme->szExePath);
		return QueryFullProcessImageNameW(hSnapshot, 0, lpme->szExePath, &pathsize);
	}
	return g_pfnModule32FirstW(hSnapshot, lpme);
}

BOOL WINAPI MyCloseHandle(HANDLE hObject)
{
	g_fakeHandleList.remove(hObject);
	return g_pfnCloseHandle(hObject);
}

void Init()
{
	Mhook_SetHook((PVOID*)&g_pfnCreateToolhelp32Snapshot, (PVOID)MyCreateToolhelp32Snapshot);
	Mhook_SetHook((PVOID*)&g_pfnModule32FirstW, (PVOID)MyModule32FirstW);
	Mhook_SetHook((PVOID*)&g_pfnCloseHandle, (PVOID)MyCloseHandle);
}