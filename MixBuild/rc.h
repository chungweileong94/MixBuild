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

	typedef vector<Point3f> PointCloud;


	void extract_image_src_set(const String& dir, ImageSrcSet& out_image_src_set);
	void extract_contours(const ImageSrcSet& image_src_set, ContoursSet& out_contours_set);
	void extract_shape(const ImageSrcSet& image_src_set, ShapeSet& out_shape_set);
	void create_othogonal_projection(const ShapeSet& shape_set, OthProjection& out_othogonal_Projection);
	void calculate_point_cloud(const OthProjection& othogonal_projection, PointCloud& out_point_cloud, const int cube_size);
	void rotate_point_cloud_y_axis(PointCloud& point_cloud, float degree);
	void transform_point_cloud(PointCloud& point_cloud, Point3f distance);


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
			auto deg_string = image_names[i].substr(start_idx, 4);
			int degree = stoi(deg_string);

			out_image_src_set[degree] = image_name;
		}
	}

	// extract contours (feature points)
	void extract_contours(const ImageSrcSet& image_src_set, ContoursSet& out_contours_set)
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

	// extract object shape
	void extract_shape(const ImageSrcSet& image_src_set, ShapeSet& out_shape_set)
	{
		// extract contours
		ContoursSet contours_set;
		extract_contours(image_src_set, contours_set);

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
		PointCloud init_point_cloud;

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
		transform_point_cloud(init_point_cloud, Point3f(-image_size.width / 2, 0, -image_size.height / 2));
		rotate_point_cloud_y_axis(init_point_cloud, -90);
		transform_point_cloud(init_point_cloud, Point3f(image_size.width / 2, 0, image_size.height / 2));

		//// finalize point cloud
		for (const auto point : init_point_cloud)
		{
			auto left_pixel = othogonal_projection.left.at<Vec3b>(point.y, point.x);
			if (left_pixel != Vec3b(0, 0, 0))
			{
				out_point_cloud.push_back(point);
			}
		}

		// adjust the origin cooordinate to 3d form
		transform_point_cloud(out_point_cloud, Point3f(-image_size.width / 2, -image_size.height / 2, -image_size.height / 2));
		rotate_point_cloud_y_axis(out_point_cloud, 90);
	}

	// point cloud Y-axis rotation
	void rotate_point_cloud_y_axis(PointCloud& point_cloud, float degree)
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
			// covert Point3f into Mat
			Mat p_mat = (Mat_<float>(4, 1) <<
				point_cloud[i].x,
				point_cloud[i].y,
				point_cloud[i].z,
				1);

			Mat result = R * p_mat;

			// assign the result back to the point cloud
			point_cloud[i] = Point3f(result.at<float>(0, 0), result.at<float>(1, 0), result.at<float>(2, 0));
		}
	}

	// point cloud transform
	void transform_point_cloud(PointCloud & point_cloud, Point3f distance)
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
			// covert Point3f into Mat
			Mat p_mat = (Mat_<float>(4, 1) <<
				point_cloud[i].x,
				point_cloud[i].y,
				point_cloud[i].z,
				1);

			Mat result = T * p_mat;

			// assign the result back to the point cloud
			point_cloud[i] = Point3f(result.at<float>(0, 0), result.at<float>(1, 0), result.at<float>(2, 0));
		}
	}




	/// Remove these codes 
	/*
	typedef vector<Point2f> Corners;
	typedef map<int, Corners> CornersSet;

	// extract corner (feature points)
	void extract_corners(const ImageSrcSet& image_src_set, CornersSet& out_corners_set)
	{
		for (auto const &img : image_src_set)
		{
			auto img_gray = imread(img.second, IMREAD_GRAYSCALE);

			Corners corners;
			goodFeaturesToTrack(img_gray, corners, 300, 0.01, 10);
			out_corners_set[img.first] = corners;
		}
	}

	// merge two cloud point
	void merge_point_cloud(PointCloud& point_cloud_new, PointCloud& out_point_cloud)
	{
		out_point_cloud.insert(out_point_cloud.end(), point_cloud_new.begin(), point_cloud_new.end());
	}

	// map corners into point cloud
	void map_corners_to_point_cloud(Corners& corners, float width, float height, float depth, PointCloud& out_point_cloud)
	{
		for (auto i = 0; i < corners.size(); i++)
		{
			out_point_cloud.push_back(Point3f(corners[i].x - width / 2, corners[i].y - height / 2, depth));
		}
	}

	bool less_by_x(const Point2f& pt_1, const Point2f& pt_2)
	{
		return pt_1.x < pt_2.x;
	}

	float get_corners_width(const Corners& corners)
	{
		auto result = std::minmax_element(corners.begin(), corners.end(), less_by_x);
		float min = corners[result.first - corners.begin()].x;
		float max = corners[result.second - corners.begin()].x;
		return std::abs(max - min);
	}
	*/
}

#endif // !RC_H