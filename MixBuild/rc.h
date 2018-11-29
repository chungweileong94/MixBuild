#pragma once
#ifndef RC_H
#define RC_H

#include <opencv2/opencv.hpp>
#include <cmath>
#include <algorithm>
#include <regex>

using namespace std;
using namespace cv;

namespace rc
{
#pragma region type_declaration

	typedef map<int, String> ImageSrcSet;

	typedef vector<Point> Contour;
	typedef vector<Contour> Contours;
	typedef map<int, Contours> ContoursSet;

	typedef Mat Shape;
	typedef map<int, Shape> ShapeSet;
	typedef struct OthProjection
	{
		Shape front;
		Shape left;
		Shape top;
	};

	typedef vector<Point3d> PointCloud;

	typedef struct PointCloudBoundary
	{
		int minX, minY, minZ, maxX, maxY, maxZ;
	};

	typedef enum PointCloudOriginForm { _2D, _3D };

	typedef vector<vector<vector<bool>>> Volume;

	typedef struct Cube
	{
		vector<bool> front;
		vector<bool> back;
	};

#pragma endregion

#pragma region methods_declaration

	void extract_image_src_set(const String& dir, ImageSrcSet& out_image_src_set);
	void extract_shape(const ImageSrcSet& image_src_set, ShapeSet& out_shape_set);
	void create_othogonal_projection(const ShapeSet& shape_set, OthProjection& out_othogonal_Projection);
	void calculate_point_cloud(const OthProjection& othogonal_projection, PointCloud& out_point_cloud, const int cube_size = 10);
	void convert_point_cloud_to_volume(const PointCloud& point_cloud, Volume& out_volume, const Size image_size);
	void __extract_contours(const ImageSrcSet& image_src_set, ContoursSet& out_contours_set);
	void __optimize_point_cloud(const PointCloud& point_cloud, PointCloud& out_point_cloud, const int cube_size, const Size image_size);
	bool __surface_condition_check(const Cube cube, const vector<bool> cube_face);
	void __convert_point_cloud_origin_form(PointCloud& point_cloud, const PointCloudOriginForm origin_form, const Size image_size);
	void __rotate_point_cloud_x_axis(PointCloud& point_cloud, float degree);
	void __rotate_point_cloud_y_axis(PointCloud& point_cloud, float degree);
	void __transform_point_cloud(PointCloud& point_cloud, Point3d distance);

#pragma endregion

#pragma region methods_definition

	// extract the image with correpond degree value from a directory
	void extract_image_src_set(const String& dir, ImageSrcSet& out_image_src_set)
	{
		vector<String> image_names;
		glob(dir, image_names);

		for (auto i = 0; i < image_names.size(); i++)
		{
			auto image_name = image_names[i];

			// the file name should be like 0045.jpg, etc.
			auto start_idx = image_name.find_last_of("\\") + 1;

			regex rx("-?\\w*\\.(jpg|jpeg|png)");
			if (!regex_match(string(image_name.substr(start_idx)), rx)) break;

			auto count = image_name.find_last_of(".") - start_idx;
			auto deg_string = image_names[i].substr(start_idx, count);
			int degree = stoi(deg_string);

			out_image_src_set[degree] = image_name;
		}
	}

	// extract object shape
	void extract_shape(const ImageSrcSet& image_src_set, ShapeSet& out_shape_set)
	{
		// extract contours
		ContoursSet contours_set;
		__extract_contours(image_src_set, contours_set);

		for (auto const &contours : contours_set)
		{
			// detect shape outline
			auto size = imread(image_src_set.at(contours.first)).size();
			Mat shape_outline = Mat::zeros(size, CV_8UC3);

			for (auto i = 0; i < contours.second.size(); i++)
			{
				drawContours(shape_outline, contours.second, i, Scalar::all(255), 1, LINE_AA);
			}

			// compute the shape fill
			Mat shape_fill = shape_outline.clone();
			floodFill(shape_fill, Point(0, 0), Scalar::all(255));
			bitwise_not(shape_fill, shape_fill);

			// merge fill and outline
			Mat shape = (shape_outline | shape_fill);
			out_shape_set[contours.first] = shape;
		}
	}

	// create othogonal projection
	void create_othogonal_projection(const ShapeSet& shape_set, OthProjection& out_othogonal_Projection)
	{
		// front
		Mat flip_180;
		flip(shape_set.at(180), flip_180, 1);
		out_othogonal_Projection.front = (shape_set.at(0) | flip_180);

		// left
		Mat flip_90;
		flip(shape_set.at(90), flip_90, 1);
		out_othogonal_Projection.left = (flip_90 | shape_set.at(270));

		// top
		out_othogonal_Projection.top = shape_set.at(-1);
	}

	// calculate point cloud
	void calculate_point_cloud(const OthProjection& othogonal_projection, PointCloud& out_point_cloud, const int cube_size)
	{
		auto image_size = othogonal_projection.front.size();
		PointCloud init_point_cloud, complete_point_cloud;

		// initial point cloud
		for (auto z = 0; z < image_size.height; z += cube_size)
		{
			for (auto y = 0; y < image_size.height; y += cube_size)
			{
				for (auto x = 0; x < image_size.width; x += cube_size)
				{
					// check if the pixel is part of the object
					auto front_pixel = othogonal_projection.front.at<Vec3b>(y, x);
					auto top_pixel = othogonal_projection.top.at<Vec3b>(z, x);

					if (front_pixel != Vec3b(0, 0, 0) && top_pixel != Vec3b(0, 0, 0))
					{
						init_point_cloud.push_back(Point3d(x, y, z));
					}
				}
			}
		}

		// rotate to left side (y-axis)
		__convert_point_cloud_origin_form(init_point_cloud, PointCloudOriginForm::_3D, image_size);
		__rotate_point_cloud_y_axis(init_point_cloud, -90);
		__convert_point_cloud_origin_form(init_point_cloud, PointCloudOriginForm::_2D, image_size);

		// finalize point cloud
		for (const auto point : init_point_cloud)
		{
			auto left_pixel = othogonal_projection.left.at<Vec3b>(point.y, point.x);
			if (left_pixel != Vec3b(0, 0, 0))
			{
				complete_point_cloud.push_back(point);
			}
		}

		// optimize point cloud (remove inner point)
		__optimize_point_cloud(complete_point_cloud, out_point_cloud, cube_size, image_size);

		// rotate back
		__convert_point_cloud_origin_form(out_point_cloud, PointCloudOriginForm::_3D, image_size);
		__rotate_point_cloud_x_axis(out_point_cloud, 180);
		//__rotate_point_cloud_y_axis(out_point_cloud, 90);
	}

	// convert point cloud to volume
	void convert_point_cloud_to_volume(const PointCloud& point_cloud, Volume& out_volume, const Size image_size)
	{
		out_volume = Volume(image_size.width, vector<vector<bool>>(image_size.height, vector<bool>(image_size.height, false)));
		for (const auto point : point_cloud)
		{
			out_volume[point.x][point.y][point.z] = true;
		}
	}

	// extract contours (feature points)
	void __extract_contours(const ImageSrcSet& image_src_set, ContoursSet& out_contours_set)
	{
		for (auto const &img : image_src_set)
		{
			auto img_gray = imread(img.second, IMREAD_GRAYSCALE);

			Mat img_detected;
			// pre-process image before canny edge detect
			blur(img_gray, img_detected, Size(3, 3));
			Canny(img_detected, img_detected, 0, 100);

			// retrieve contours
			Contours contours;
			findContours(img_detected, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);
			out_contours_set[img.first] = contours;
		}
	}

	// remove inner point cloud & optimize for surface rendering
	void __optimize_point_cloud(const PointCloud& point_cloud, PointCloud& out_point_cloud, const int cube_size, const Size image_size)
	{
		// find the model volume size
		bool initialize = false;

		PointCloudBoundary boundary;
		for (const auto point : point_cloud)
		{
			boundary.minX = !initialize || point.x < boundary.minX ? point.x : boundary.minX;
			boundary.minY = !initialize || point.y < boundary.minY ? point.y : boundary.minY;
			boundary.minZ = !initialize || point.z < boundary.minZ ? point.z : boundary.minZ;

			boundary.maxX = !initialize || point.x > boundary.maxX ? point.x : boundary.maxX;
			boundary.maxY = !initialize || point.y > boundary.maxY ? point.y : boundary.maxY;
			boundary.maxZ = !initialize || point.z > boundary.maxZ ? point.z : boundary.maxZ;
			initialize = true;
		}

		// create a model volume
		Volume volume;
		convert_point_cloud_to_volume(point_cloud, volume, image_size);

		for (auto x = boundary.minX; x < boundary.maxX; x += cube_size)
		{
			for (auto y = boundary.minY; y < boundary.maxY; y += cube_size)
			{
				for (auto z = boundary.minZ; z < boundary.maxZ; z += cube_size)
				{
#pragma region front
					Cube cube
					{
						vector<bool>{
							volume[x][y][z],
							volume[x + cube_size][y][z],
							volume[x + cube_size][y + cube_size][z],
							volume[x][y + cube_size][z]
						},
						vector<bool>
						{
							volume[x][y][z + cube_size],
							volume[x + cube_size][y][z + cube_size],
							volume[x + cube_size][y + cube_size][z + cube_size],
							volume[x][y + cube_size][z + cube_size]
						}
					};

					vector<bool> cube_face
					{
						volume[x][y][z - cube_size],
						volume[x + cube_size][y][z - cube_size],
						volume[x + cube_size][y + cube_size][z - cube_size],
						volume[x][y + cube_size][z - cube_size]
					};

					if (__surface_condition_check(cube, cube_face))
					{
						out_point_cloud.push_back(Point3d(x, y, z));
						out_point_cloud.push_back(Point3d(x + cube_size, y, z));
						out_point_cloud.push_back(Point3d(x + cube_size, y + cube_size, z));
						out_point_cloud.push_back(Point3d(x, y + cube_size, z));
					}
#pragma endregion

#pragma region back
					cube = Cube
					{
						vector<bool>
						{
							volume[x + cube_size][y][z + cube_size],
							volume[x][y][z + cube_size],
							volume[x][y + cube_size][z + cube_size],
							volume[x + cube_size][y + cube_size][z + cube_size]
						},
						vector<bool>
						{
							volume[x + cube_size][y][z],
							volume[x][y][z],
							volume[x][y + cube_size][z],
							volume[x + cube_size][y + cube_size][z]
						}
					};

					cube_face = vector<bool>
					{
						volume[x + cube_size][y][z + cube_size * 2],
						volume[x][y][z + cube_size * 2],
						volume[x][y + cube_size][z + cube_size * 2],
						volume[x + cube_size][y + cube_size][z + cube_size * 2]
					};

					if (__surface_condition_check(cube, cube_face))
					{
						out_point_cloud.push_back(Point3d(x + cube_size, y, z + cube_size));
						out_point_cloud.push_back(Point3d(x, y, z + cube_size));
						out_point_cloud.push_back(Point3d(x, y + cube_size, z + cube_size));
						out_point_cloud.push_back(Point3d(x + cube_size, y + cube_size, z + cube_size));
					}
#pragma endregion

#pragma region left
					cube = Cube
					{
						vector<bool>
						{
							volume[x][y][z + cube_size],
							volume[x][y][z],
							volume[x][y + cube_size][z],
							volume[x][y + cube_size][z + cube_size]
						},
						vector<bool>
						{
							volume[x + cube_size][y][z + cube_size],
							volume[x + cube_size][y][z],
							volume[x + cube_size][y + cube_size][z],
							volume[x + cube_size][y + cube_size][z + cube_size]
						}
					};

					cube_face = vector<bool>
					{
						volume[x - cube_size][y][z + cube_size],
						volume[x - cube_size][y][z],
						volume[x - cube_size][y + cube_size][z],
						volume[x - cube_size][y + cube_size][z + cube_size]
					};

					if (__surface_condition_check(cube, cube_face))
					{
						out_point_cloud.push_back(Point3d(x, y, z + cube_size));
						out_point_cloud.push_back(Point3d(x, y, z));
						out_point_cloud.push_back(Point3d(x, y + cube_size, z));
						out_point_cloud.push_back(Point3d(x, y + cube_size, z + cube_size));
					}
#pragma endregion

#pragma region right
					cube = Cube
					{
						vector<bool>
						{
							volume[x + cube_size][y][z],
							volume[x + cube_size][y][z + cube_size],
							volume[x + cube_size][y + cube_size][z + cube_size],
							volume[x + cube_size][y + cube_size][z]
						},
						vector<bool>
						{
							volume[x][y][z],
							volume[x][y][z + cube_size],
							volume[x][y + cube_size][z + cube_size],
							volume[x][y + cube_size][z]
						}
					};

					cube_face = vector<bool>
					{
						volume[x + cube_size * 2][y][z],
						volume[x + cube_size * 2][y][z + cube_size],
						volume[x + cube_size * 2][y + cube_size][z + cube_size],
						volume[x + cube_size * 2][y + cube_size][z]
					};

					if (__surface_condition_check(cube, cube_face))
					{
						out_point_cloud.push_back(Point3d(x + cube_size, y, z));
						out_point_cloud.push_back(Point3d(x + cube_size, y, z + cube_size));
						out_point_cloud.push_back(Point3d(x + cube_size, y + cube_size, z + cube_size));
						out_point_cloud.push_back(Point3d(x + cube_size, y + cube_size, z));
					}
#pragma endregion

#pragma region top
					cube = Cube
					{
						vector<bool>
						{
							volume[x][y][z + cube_size],
							volume[x + cube_size][y][z + cube_size],
							volume[x + cube_size][y][z],
							volume[x][y][z]
						},
						vector<bool>
						{
							volume[x][y + cube_size][z + cube_size],
							volume[x + cube_size][y + cube_size][z + cube_size],
							volume[x + cube_size][y + cube_size][z],
							volume[x][y + cube_size][z]
						}
					};

					cube_face = vector<bool>
					{
						volume[x][y - cube_size][z + cube_size],
						volume[x + cube_size][y - cube_size][z + cube_size],
						volume[x + cube_size][y - cube_size][z],
						volume[x][y - cube_size][z]
					};

					if (__surface_condition_check(cube, cube_face))
					{
						out_point_cloud.push_back(Point3d(x, y, z + cube_size));
						out_point_cloud.push_back(Point3d(x + cube_size, y, z + cube_size));
						out_point_cloud.push_back(Point3d(x + cube_size, y, z));
						out_point_cloud.push_back(Point3d(x, y, z));
					}
#pragma endregion
				}
			}
		}
	}

	bool __surface_condition_check(const Cube cube, const vector<bool> cube_face)
	{
		bool must_condition = cube.back[0] && cube.back[1] && cube.back[2] && cube.back[3] &&
			cube.front[0] && cube.front[1] && cube.front[2] && cube.front[3];

		bool condition_1 = !cube_face[0] && !cube_face[1] && !cube_face[2] && !cube_face[3];
		bool condition_2 = !cube_face[0] && !cube_face[1] && cube_face[2] && cube_face[3];
		bool condition_3 = cube_face[0] && cube_face[1] && !cube_face[2] && !cube_face[3];
		bool condition_4 = !cube_face[0] && cube_face[1] && cube_face[2] && !cube_face[3];
		bool condition_5 = cube_face[0] && !cube_face[1] && !cube_face[2] && cube_face[3];

		bool condition_6 = cube_face[0] && !cube_face[1] && !cube_face[2] && !cube_face[3];
		bool condition_7 = !cube_face[0] && cube_face[1] && !cube_face[2] && !cube_face[3];
		bool condition_8 = !cube_face[0] && !cube_face[1] && cube_face[2] && !cube_face[3];
		bool condition_9 = !cube_face[0] && !cube_face[1] && !cube_face[2] && cube_face[3];

		return must_condition && (condition_1 || condition_2 || condition_3 || condition_4 || condition_5 || condition_6 || condition_7 || condition_8 || condition_9);
	}

	// covert point cloud origin form (2D <-> 3D)
	void __convert_point_cloud_origin_form(PointCloud& point_cloud, const PointCloudOriginForm origin_form, const Size image_size)
	{
		int t_x = origin_form == PointCloudOriginForm::_3D ? -image_size.width / 2 : image_size.width / 2;
		int t_y = origin_form == PointCloudOriginForm::_3D ? -image_size.height / 2 : image_size.height / 2;
		int t_z = origin_form == PointCloudOriginForm::_3D ? -image_size.height / 2 : image_size.height / 2;
		__transform_point_cloud(point_cloud, Point3d(t_x, t_y, t_z));
	}

	// point cloud X-axis rotation
	void __rotate_point_cloud_x_axis(PointCloud& point_cloud, float degree)
	{
		float beta = degree * CV_PI / 180;

		// Y-axis rotation matrix
		Mat R = (Mat_<float>(4, 4) <<
			1, 0, 0, 0,
			0, cos(beta), -sin(beta), 0,
			0, sin(beta), cos(beta), 0,
			0, 0, 0, 1);

		// perform rotation to the point cloud
		for (auto i = 0; i < point_cloud.size(); i++)
		{
			// covert Point3d into Mat
			Mat p_mat = (Mat_<float>(4, 1) <<
				point_cloud[i].x,
				point_cloud[i].y,
				point_cloud[i].z,
				1);

			Mat result = R * p_mat;

			// assign the result back to the point cloud
			point_cloud[i] = Point3d(result.at<float>(0, 0), result.at<float>(1, 0), result.at<float>(2, 0));
		}
	}

	// point cloud Y-axis rotation
	void __rotate_point_cloud_y_axis(PointCloud& point_cloud, float degree)
	{
		float beta = degree * CV_PI / 180;

		// Y-axis rotation matrix
		Mat R = (Mat_<float>(4, 4) <<
			cos(beta), 0, -sin(beta), 0,
			0, 1, 0, 0,
			sin(beta), 0, cos(beta), 0,
			0, 0, 0, 1);

		// perform rotation to the point cloud
		for (auto i = 0; i < point_cloud.size(); i++)
		{
			// covert Point3d into Mat
			Mat p_mat = (Mat_<float>(4, 1) <<
				point_cloud[i].x,
				point_cloud[i].y,
				point_cloud[i].z,
				1);

			Mat result = R * p_mat;

			// assign the result back to the point cloud
			point_cloud[i] = Point3d(result.at<float>(0, 0), result.at<float>(1, 0), result.at<float>(2, 0));
		}
	}

	// point cloud transform
	void __transform_point_cloud(PointCloud & point_cloud, Point3d distance)
	{
		// transformation matrix
		Mat T = (Mat_<float>(4, 4) <<
			1, 0, 0, distance.x,
			0, 1, 0, distance.y,
			0, 0, 1, distance.z,
			0, 0, 0, 1);

		// perform transformation to the point cloud
		for (auto i = 0; i < point_cloud.size(); i++)
		{
			// covert Point3d into Mat
			Mat p_mat = (Mat_<float>(4, 1) <<
				point_cloud[i].x,
				point_cloud[i].y,
				point_cloud[i].z,
				1);

			Mat result = T * p_mat;

			// assign the result back to the point cloud
			point_cloud[i] = Point3d(result.at<float>(0, 0), result.at<float>(1, 0), result.at<float>(2, 0));
		}
	}

#pragma endregion
}

#endif // !RC_H