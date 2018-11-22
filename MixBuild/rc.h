#pragma once
#ifndef RC_H
#define RC_H

#include <opencv2/opencv.hpp>
#include <cmath>
#include <algorithm>

using namespace std;
using namespace cv;

namespace rc
{
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

	typedef enum PointCloudOriginForm { _2D, _3D };

	typedef vector<vector<vector<bool>>> Volume;


	void extract_image_src_set(const String& dir, ImageSrcSet& out_image_src_set);
	void extract_shape(const ImageSrcSet& image_src_set, ShapeSet& out_shape_set);
	void create_othogonal_projection(const ShapeSet& shape_set, OthProjection& out_othogonal_Projection);
	void calculate_point_cloud(const OthProjection& othogonal_projection, PointCloud& out_point_cloud, const int cube_size);
	void __extract_contours(const ImageSrcSet& image_src_set, ContoursSet& out_contours_set);
	void __optimize_point_cloud(const PointCloud& point_cloud, PointCloud& out_point_cloud, const int cube_size, const Size image_size);
	void __convert_point_cloud_origin_form(PointCloud& point_cloud, const PointCloudOriginForm origin_form, const Size image_size);
	void __rotate_point_cloud_y_axis(PointCloud& point_cloud, float degree);
	void __transform_point_cloud(PointCloud& point_cloud, Point3d distance);


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
	void calculate_point_cloud(const OthProjection& othogonal_projection, PointCloud& out_point_cloud, const int cube_size = 5)
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
		__rotate_point_cloud_y_axis(out_point_cloud, 90);
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

	// remove inner point cloud
	void __optimize_point_cloud(const PointCloud& point_cloud, PointCloud& out_point_cloud, const int cube_size, const Size image_size)
	{
		// find the model volume size
		int minX, minY, minZ, maxX, maxY, maxZ;
		bool initialize = false;

		for (const auto point : point_cloud)
		{
			minX = !initialize || point.x < minX ? point.x : minX;
			minY = !initialize || point.y < minY ? point.y : minY;
			minZ = !initialize || point.z < minZ ? point.z : minZ;

			maxX = !initialize || point.x > maxX ? point.x : maxX;
			maxY = !initialize || point.y > maxY ? point.y : maxY;
			maxZ = !initialize || point.z > maxZ ? point.z : maxZ;
			initialize = true;
		}

		// create a model volume
		Volume volume(image_size.width, vector<vector<bool>>(image_size.height, vector<bool>(image_size.height, false)));
		for (const auto point : point_cloud)
		{
			volume[point.x][point.y][point.z] = true;
		}

		// retrieve only the surface points
		/// front & back
		for (auto x = minX; x <= maxX; x += cube_size)
		{
			for (auto y = minY; y <= maxY; y += cube_size)
			{
				for (auto z = minZ; z <= maxZ; z += cube_size)
				{
					if (volume[x][y][z])
					{
						out_point_cloud.push_back(Point3d(x, y, z));
						break;
					}
				}
				for (auto z = maxZ; z >= minZ; z -= cube_size)
				{
					if (volume[x][y][z])
					{
						out_point_cloud.push_back(Point3d(x, y, z));
						break;
					}
				}
			}
		}

		/// left & right
		for (auto z = minZ; z <= maxZ; z += cube_size)
		{
			for (auto y = minY; y <= maxY; y += cube_size)
			{
				for (auto x = minX; x <= maxX; x += cube_size)
				{
					if (volume[x][y][z])
					{
						out_point_cloud.push_back(Point3d(x, y, z));
						break;
					}
				}
				for (auto x = maxX; x >= minX; x -= cube_size)
				{
					if (volume[x][y][z])
					{
						out_point_cloud.push_back(Point3d(x, y, z));
						break;
					}
				}
			}
		}

		/// top & bottom
		for (auto z = minZ; z <= maxZ; z += cube_size)
		{
			for (auto x = minX; x <= maxX; x += cube_size)
			{
				for (auto y = minY; y <= maxY; y += cube_size)
				{
					if (volume[x][y][z])
					{
						out_point_cloud.push_back(Point3d(x, y, z));
						break;
					}
				}

				for (auto y = maxY; y >= minY; y -= cube_size)
				{
					if (volume[x][y][z])
					{
						out_point_cloud.push_back(Point3d(x, y, z));
						break;
					}
				}
			}
		}
	}

	// covert point cloud origin form (2D <-> 3D)
	void __convert_point_cloud_origin_form(PointCloud& point_cloud, const PointCloudOriginForm origin_form, const Size image_size)
	{
		int t_x = origin_form == PointCloudOriginForm::_3D ? -image_size.width / 2 : image_size.width / 2;
		int t_y = origin_form == PointCloudOriginForm::_3D ? -image_size.height / 2 : image_size.height / 2;
		int t_z = origin_form == PointCloudOriginForm::_3D ? -image_size.height / 2 : image_size.height / 2;
		__transform_point_cloud(point_cloud, Point3d(t_x, t_y, t_z));
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
}

#endif // !RC_H