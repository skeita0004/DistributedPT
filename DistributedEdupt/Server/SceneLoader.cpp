#include "SceneLoader.h"

#include <fstream>
#include <filesystem>

#include <json.hpp>

SceneLoader::SceneLoader()
{}

SceneLoader::~SceneLoader()
{}

bool SceneLoader::Load(const std::string& _fileName)
{
	using json = nlohmann::json;
	
	std::ifstream sceneFile(_fileName);
	if(sceneFile.good())
	{
		// ファイルサイズの取得
		sceneFile.seekg(0, std::ios_base::end);
		int buffSize = sceneFile.tellg();
		sceneFile.seekg(0, std::ios_base::beg);

		std::string fileData{};

		while(std::getline(sceneFile, fileData))
		{
		}
		
		// charの配列にコピー
		std::copy(fileData.begin(), fileData.end(), sceneData_.begin());
	}

    return false;
}
