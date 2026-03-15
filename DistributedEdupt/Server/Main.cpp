#include "Application.hpp"
#include <iostream>

int main(int argc, char** argv)
{
	// cのスタイルはここで終了
	std::vector<std::string> args(argv, argv + argc);

	Application app{};

	int result = app.Run(args);
	if (result != 0)
	{
		std::cerr << "app.Run failed." << std::endl;
		return -1;
	}

	return 0;
}