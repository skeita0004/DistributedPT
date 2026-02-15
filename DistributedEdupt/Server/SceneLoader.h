#pragma once

#include <string>
#include <vector>

class SceneLoader
{
public:
	SceneLoader();
	~SceneLoader();
	
	bool Load(const std::string& _fileName);
	
	std::vector<char> GetData()
	{
		return sceneData_;
	}

private:
	std::vector<char> sceneData_;

};