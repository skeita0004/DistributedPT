#pragma once
#include <string>
#include <Windows.h>

class Viewer
{
public:
	Viewer();

	/// @brief ローカルクライアントの起動
	/// @return 成功:true, 失敗:false
	bool Launch(const std::string& _exePath,
				const std::string& _imageWidth,
				const std::string& _imageHeight,
				const std::string& _tileNumX,
				const std::string& _tileNumY,
				const std::string& _tileSize);

	/// @brief ローカルクライアントの終了
	void Terminate();

private:
	PROCESS_INFORMATION procInfo_;
};