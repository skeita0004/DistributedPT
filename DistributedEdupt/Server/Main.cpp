#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include "Server.h"
#include <conio.h>
#include "ppm.h"

int main(int argc, char** argv)
{
	// 引数チェック(解像度、スーパーサンプル数、サンプル数)
	if (argc != 5)
	{
		std::cerr << "Invalid argment." << std::endl;
		std::cerr << "Example: ./DEduptSV.exe width height supersample sample" << std::endl;
		return -1;
	}

	Server server{};
	if (server.Initialize(argv) != 0)
	{
		std::cerr << "Initialize failed." << std::endl;
		return -1;
	}

	// --- クライアント接続受付フェーズ ---
	std::cout << "Waiting Client..." << std::endl;
	while (true)
	{
		server.JoinClient();

		if (_kbhit() == TRUE)
		{
			if (_getch() == 13)
			{
				break;
			}
		}

		Sleep(100);
	}
	std::cout << "\nStop Accept!" << std::endl;
	std::cout << "Transitioning to the calculation phase..." << std::endl;

	server.PreparationSendData();
	server.SendData();

	std::cout << "全てのレンダリングデータの送信が完了しました" << std::endl;

	// 受信(待機)、画像合成、形式変換(ffmpeg)
	int counter{0};
	while (true)
	{
		Sleep(100);
		// 1.送信したタイルの数分、タイルを受信できるまで受信処理
		server.RecvData();

		if (server.GetRecvDataNum() >= server.GetTotalTileNum())
		{
			break;
		}

	}

	int i = 0;
	for (auto& tile : server.GetRenderResult())
	{
		edupt::save_ppm_file("out" + std::to_string(i) + ".ppm", tile.renderResult.data(), 64, 64);
		i++;
	}

	// 2.1.が終わったら、画像を合成し、リサイズ、形式変換
	system(server.GetffmpegArgs().c_str());

	server.Release();

	std::cout << "Press any key to exit." << std::endl;
	_getch();

	return 0;
}