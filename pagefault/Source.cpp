// A short program to demonstrate dynamic memory allocation
// using a structured exception handler.

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>             // For exit

long  PAGELIMIT = 10;            // Number of pages to ask for

LPTSTR lpNxtPage;               // Address of the next page to ask for
DWORD dwPages = 0;              // Count of pages gotten so far
DWORD dwPageSize;               // Page size on this computer

long g_pagecouts = 0;

INT PageFaultExceptionFilter(DWORD dwCode)
{
	LPVOID lpvResult;

	// If the exception is not a page fault, exit.

	if (dwCode != EXCEPTION_ACCESS_VIOLATION)
	{
		_tprintf(TEXT("Exception code = %d.\n"), dwCode);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	//_tprintf(TEXT("Exception is a page fault.\n"));

	// If the reserved pages are used up, exit.

	if (dwPages >= PAGELIMIT)
	{
		_tprintf(TEXT("Exception: out of pages.\n"));
		return EXCEPTION_EXECUTE_HANDLER;
	}

	// Otherwise, commit another page.

	lpvResult = VirtualAlloc(
		(LPVOID)lpNxtPage, // Next page to commit
		dwPageSize,         // Page size, in bytes
		MEM_COMMIT,         // Allocate a committed page
		PAGE_READWRITE);    // Read/write access
	if (lpvResult == NULL)
	{
		//_tprintf(TEXT("VirtualAlloc failed.\n"));
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
	{
		//_tprintf(TEXT("Allocating another page.\n"));
	}

	// Increment the page count, and advance lpNxtPage to the next page.

	dwPages++;
	lpNxtPage = (LPTSTR)((PCHAR)lpNxtPage + dwPageSize);

	// Continue execution where the page fault occurred.

	return EXCEPTION_CONTINUE_EXECUTION;
}

VOID ErrorExit(LPTSTR lpMsg)
{
	_tprintf(TEXT("Error! %s with error code of %ld.\n"),
		lpMsg, GetLastError());
	exit(0);
}
void main(int arg, char** argc)
{
	LPVOID lpvBase;               // Base address of the test memory
	LPTSTR lpPtr;                 // Generic character pointer
	BOOL bSuccess;                // Flag
	DWORD i;                      // Generic counter
	SYSTEM_INFO sSysInfo;         // Useful information about the system
	g_pagecouts = 0;
	if (argc[1] != NULL)
	{
		g_pagecouts = atol(argc[1]);
		//printf("%d \n", g_pagecouts);
		if (g_pagecouts != 0) {
			PAGELIMIT = g_pagecouts;
		}
	}
	else
	{
		PAGELIMIT = 100;
	}
	GetSystemInfo(&sSysInfo);     // Initialize the structure.

	_tprintf(TEXT("This computer has page size %d.\n"), sSysInfo.dwPageSize);

	dwPageSize = sSysInfo.dwPageSize;

	// Reserve pages in the virtual address space of the process.

	lpvBase = VirtualAlloc(
		NULL,                 // System selects address
		PAGELIMIT*dwPageSize, // Size of allocation
		MEM_RESERVE,          // Allocate reserved pages
		PAGE_NOACCESS);       // Protection = no access
	if (lpvBase == NULL)
		ErrorExit(LPTSTR("VirtualAlloc reserve failed."));

	lpPtr = lpNxtPage = (LPTSTR)lpvBase;

	// Use structured exception handling when accessing the pages.
	// If a page fault occurs, the exception filter is executed to
	// commit another page from the reserved block of pages.
	//统计时间
	LARGE_INTEGER timestart;
	LARGE_INTEGER timeend;
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&timestart);
	double quadpart = (double)frequency.QuadPart; //frequency
	//DWORD start = GetTickCount();
	for (i = 0; i < PAGELIMIT*dwPageSize; i++)
	{
		__try
		{
			// Write to memory.

			lpPtr[i] = 'a';
		}

		// If there's a page fault, commit another page and try again.

		__except (PageFaultExceptionFilter(GetExceptionCode()))
		{

			// This code is executed only if the filter function
			// is unsuccessful in committing the next page.

			_tprintf(TEXT("Exiting process.\n"));

			ExitProcess(GetLastError());

		}

	}
	//DWORD  end = GetTickCount(); //单位ms
	//printf("run time is %d ms\n", end - start);
	QueryPerformanceCounter(&timeend);
	double elapsed = (timeend.QuadPart - timestart.QuadPart) / quadpart; //单位为s 精度为（1000000/cpu主频）
	printf("run time is %f s\n", elapsed);
	// Release the block of pages when you are finished using them.
	bSuccess = VirtualFree(
		lpvBase,       // Base address of block
		0,             // Bytes of committed pages
		MEM_RELEASE);  // Decommit the pages

	_tprintf(TEXT("Release %s.\n"), bSuccess ? TEXT("succeeded") : TEXT("failed"));
	system("pause");
}