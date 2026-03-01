#ifndef _RENDER_H_
#define _RENDER_H_

#include <iostream>
#include <vector>

#include "radiance.h"
#include "ppm.h"
#include "random.h"
#include "render_data.h"

#include <omp.h>

namespace edupt
{
	int render(RenderData _renderData, std::vector<Color>& _image)
	{
		// カメラ位置(共通)
		const Vec camera_position = Vec(50.0, 52.0, 220.0);
		const Vec camera_dir = normalize(Vec(0.0, -0.04, -1.0));
		const Vec camera_up = Vec(0.0, 1.0, 0.0);

		// ワールド座標系でのスクリーンの大きさ(共通)
		const double screen_width = 30.0 * _renderData.width / _renderData.height;
		const double screen_height = 30.0;

		// スクリーンまでの距離(共通)
		const double screen_dist = 40.0;

		// スクリーンを張るベクトル(共通)
		const Vec screen_x = normalize(cross(camera_dir, camera_up)) * screen_width;
		const Vec screen_y = normalize(cross(screen_x, camera_dir)) * screen_height;
		const Vec screen_center = camera_position + camera_dir * screen_dist;

		// レンダリング結果格納配列(非共通)

		// TODO: OpenMPの追加

		// グローバル座標ベース
		int tileBottomY = _renderData.offsetY + _renderData.tileHeight;
		int tileRightX  = _renderData.offsetX + _renderData.tileWidth;

		#pragma omp parallel for schedule(dynamic, 1) num_threads(12)
		for (int y = _renderData.offsetY; y < tileBottomY; y++)
		{
			std::cerr << "Rendering (y = " << y << ") " << (100.0 * y / (tileBottomY - 1)) << "%" << std::endl;

			for (int x = _renderData.offsetX; x < tileRightX; x++)
			{
				int localY = y - _renderData.offsetY;
				int localX = x - _renderData.offsetX;

				// 左下原点にする
				//const int image_index = (_renderData.tileHeight - localY - 1) * _renderData.tileWidth + localX;
				const int image_index = _renderData.tileWidth * localY + localX;

				uint32_t seed = y * _renderData.width + x;
				Random rnd(seed + 1);

				// _renderData.superSample x _renderData.superSample のスーパーサンプリング
				// ここはタイルごとに同じ
				for (int sy = 0; sy < _renderData.superSample; sy++)
				{
					for (int sx = 0; sx < _renderData.superSample; sx++)
					{
						Color accumulated_radiance = Color();

						// 一つのサブピクセルあたり_renderData.sample回サンプリングする
						// ここもタイルごとに同じ
						for (int s = 0; s < _renderData.sample; s++)
						{
							const double rate = (1.0 / _renderData.superSample);
							const double r1 = sx * rate + rate / 2.0;
							const double r2 = sy * rate + rate / 2.0;
							// スクリーン上の位置
							const Vec screen_position =
								screen_center +
								screen_x * ((r1 + x) / _renderData.width - 0.5) +
								screen_y * ((r2 + y) / _renderData.height - 0.5);
							// レイを飛ばす方向
							const Vec dir = normalize(screen_position - camera_position);

							accumulated_radiance = accumulated_radiance +
								radiance(Ray(camera_position, dir), &rnd, 0) / _renderData.sample / (_renderData.superSample * _renderData.superSample);
						}

						// ここで、値を一時的な配列に格納する
						// もちろんタイル単位
						_image[image_index] = _image[image_index] + accumulated_radiance;
					}
				}
			}
		}
		// これは、サーバ側から呼ぶ(ここでは呼ばない)
		// save_ppm_file(std::string("image.ppm"), image, _renderData.width, _renderData.height);
		return 0;
	}
};

#endif
