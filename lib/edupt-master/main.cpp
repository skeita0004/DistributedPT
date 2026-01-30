#include <iostream>

#include "render.h"

int main(int argc, char** argv)
{
	std::cout << "Path tracing renderer: edupt" << std::endl << std::endl;

	// 640x480の画像、(2x2) * 4 sample / pixel

	// 画像の大きさを得た後に、
	// サーバ、クライアントはこいつを呼ぶ
	edupt::render(640, 480, 16, 8);
}
