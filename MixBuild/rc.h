#pragma once
#ifndef RC_H
#define RC_H

#include <opencv2/opencv.hpp>
#include <cmath>

using namespace std;
using namespace cv;

namespace rc
{
	typedef vector<Point> Contour;
	typedef vector<Contour> Contours;
	typedef vector<Point2f> Corners;
	typedef vector<Point3f> PointCloud;

	// extract contours (feature points)
	void extract_contours(const vector<String>& image_names, vector<Contours>& out_contours_collection)
	{
		for (auto i = 0; i < image_names.size(); i++)
		{
			auto img_gray = imread(image_names[i], IMREAD_GRAYSCALE);

			Mat img_detected;
			// pre-process image before canny edge detect
			blur(img_gray, img_detected, Size(3, 3));
			Canny(img_detected, img_detected, 0, 100);

			// retrieve contours
			Contours contours;
			findContours(img_detected, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);
			out_contours_collection.push_back(contours);
		}
	}

	// extract corner (feature points)
	void extract_corners(const vector<String>& image_names, vector<Corners>& out_corners_collection)
	{
		for (auto i = 0; i < image_names.size(); i++)
		{
			auto img_gray = imread(image_names[i], IMREAD_GRAYSCALE);

			Corners corners;
			goodFeaturesToTrack(img_gray, corners, 300, 0.01, 10);
			out_corners_collection.push_back(corners);
		}
	}

	// point cloud Y-axis rotation
	void rotate_y_axis(PointCloud& point_cloud, float degree)
	{
		float beta = degree * CV_PI / 180;

		// Y-axis rotation matrix
		Mat R = (Mat_<float>(4, 4) <<
			cos(beta), 0, sin(beta), 0,
			0, 1, 0, 0,
			-sin(beta), 0, cos(beta), 0,
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

	// merge two cloud point
	void merge_point_cloud(PointCloud& point_cloud_new, PointCloud& out_point_cloud)
	{
		out_point_cloud.insert(out_point_cloud.end(), point_cloud_new.begin(), point_cloud_new.end());
	}

	// map corners into point cloud
	void map_corners_to_point_cloud(vector<Point2f>& corners, float width, float height, PointCloud& out_point_cloud)
	{
		for (auto i = 0; i < corners.size(); i++)
		{
			out_point_cloud.push_back(Point3f(corners[i].x - width / 2, corners[i].y - height / 2, 1));
		}
	}
}

#endif // !RC_H