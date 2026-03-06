#include "SubProcess.h"

#include <iostream>

SubProcess::SubProcess() :
	procInfo_()
{
}

SubProcess::~SubProcess()
{
	if (CloseHandle(procInfo_.hProcess) == FALSE)
	{
		std::cerr << "FAILED TO CloseHandle(hProcess) " << std::endl;
	}

	if (CloseHandle(procInfo_.hThread) == FALSE)
	{
		std::cerr << "FAILED TO CloseHandle(hThread) " << std::endl;
	}
}

bool SubProcess::Launch(std::string _commandLine,
						DWORD _creationFlags)
{
	bool result{true};

	STARTUPINFOA startUpInfo{};
	startUpInfo.cb = sizeof(startUpInfo);
	result = static_cast<bool>(CreateProcessA(
							   nullptr,
							   _commandLine.data(),
							   nullptr,
							   nullptr,
							   FALSE,
							   _creationFlags,
							   nullptr,
							   nullptr,
							   &startUpInfo,
							   &procInfo_));

	if (result == false)
	{
		DWORD err = GetLastError();
		std::cerr << "CreateProcessA failed. ERROR = " << err << std::endl;
	}

	return result;
}