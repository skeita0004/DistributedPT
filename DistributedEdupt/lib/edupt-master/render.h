#ifndef _RENDER_H_
#define _RENDER_H_

#include <iostream>

#include "radiance.h"
#include "ppm.h"
#include "random.h"
#include "render_data.h"

#include <omp.h>

namespace edupt
{
	// この辺をいじっていく必要がある(齋藤より)
	// 引数長すぎなので、構造体を作る
	int render(RenderData _renderData)
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
		Color* image = new Color[_renderData.width * _renderData.height];

		//std::cout << _renderData.width << "x" << _renderData.height << " " << _renderData.sample * (_renderData.superSample * _renderData.superSample) << " spp" << std::endl;

		// OpenMP (検証用にいったんオフ)
//		omp_set_num_threads(12);
//#pragma omp parallel for schedule(static)

		// ここを変える
		for (int y = 0; y < _renderData.height; y++)
		{
			// 進捗度表示
			//std::cerr << "Rendering (y = " << y << ") " << (100.0 * y / (_renderData.height - 1)) << "%" << std::endl;
			//printf("thread = %d\n", omp_get_thread_num());

			// ここと
			// ノイズがタイルごとに変わるので、画像の切れ目がわかってしまう。
			Random rnd(y + 1);
			
			// ここも
			for (int x = 0; x < _renderData.width; x++)
			{
				const int image_index = (_renderData.tileHeight - y - 1) * _renderData.tileWidth + x;
				
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
						image[image_index] = image[image_index] + accumulated_radiance;

						// return data
						_renderData.imageArray[image_index] = image_index[image_index] + accumulated_radiance;
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
