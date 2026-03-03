#include "Viewer.h"

#include <iostream>

Viewer::Viewer() :
	procInfo_()
{
}

bool Viewer::Launch(const std::string& _exePath,
					const std::string& _imageWidth,
					const std::string& _imageHeight,
					const std::string& _tileNumX,
					const std::string& _tileNumY,
					const std::string& _tileSize)
{
	std::string cmdLine{_exePath + " " + _imageWidth + " " + _imageHeight + " " + _tileNumX + " " + _tileNumY + " " + _tileSize};

	STARTUPINFOA startUpInfo{};
	startUpInfo.cb = sizeof(startUpInfo);

	DWORD creationFlags{CREATE_NEW_CONSOLE};

	BOOL result{};
	result = CreateProcessA(nullptr,
							cmdLine.data(),
							nullptr,
							nullptr,
							FALSE,
							creationFlags,
							nullptr,
							nullptr,
							&startUpInfo,
							&procInfo_);

	if (result == FALSE)
	{
		DWORD err = GetLastError();
		std::cerr << "CreateProcessA failed. ERROR = " << err << std::endl;
	}

	return result;
}

void Viewer::Terminate()
{
	if (CloseHandle(procInfo_.hProcess) == FALSE)
	{
		std::cerr << "FAILED TO CloseHandle(hThread) " << "line:" << __LINE__ << std::endl;
	}

	if (CloseHandle(procInfo_.hThread) == FALSE)
	{
		std::cerr << "FAILED TO CloseHandle(hThread) " << "line:" << __LINE__ << std::endl;
	}
}
