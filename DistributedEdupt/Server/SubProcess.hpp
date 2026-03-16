#pragma once
#include <string>
#include <Windows.h>

class SubProcess
{
public:
	SubProcess();
	~SubProcess() noexcept;

	/// @brief ローカルクライアントの起動
	/// @return 成功:true, 失敗:false
	bool Launch(std::string _commandLine,
				DWORD _creationFlags);

private:
	PROCESS_INFORMATION procInfo_;
};