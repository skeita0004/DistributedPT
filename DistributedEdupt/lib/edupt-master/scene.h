#ifndef	_SCENE_H_
#define	_SCENE_H_

#include "constant.h"
#include "sphere.h"
#include "intersection.h"

#include <vector>

namespace edupt
{
	// レンダリングするシーンデータ
	std::vector<Sphere> sceneData;

	inline void AddObject(const Sphere& _sphere)
	{
		sceneData.push_back(_sphere);
	}

	// シーンとの交差判定関数
	inline bool intersect_scene(const Ray& ray, Intersection* intersection)
	{
		const double n = sceneData.size();

		// 初期化
		intersection->hitpoint.distance = kINF;
		intersection->object_id = -1;

		// 線形探索
		for (int i = 0; i < int(n); i++)
		{
			Hitpoint hitpoint;
			if (sceneData[i].intersect(ray, &hitpoint))
			{
				if (hitpoint.distance < intersection->hitpoint.distance)
				{
					intersection->hitpoint = hitpoint;
					intersection->object_id = i;
				}
			}
		}

		return (intersection->object_id != -1);
	}

};

#endif
