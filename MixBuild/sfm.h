#pragma once

#ifndef SFM_H
#define SFM_H

#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

using namespace std;
using namespace cv;

namespace sfm
{
	// extract feature points
	void extract_features(
		vector<String>& image_names,
		vector<vector<KeyPoint>>& out_key_points_all,
		vector<Mat>& out_descriptor_all,
		vector<vector<Vec3b>>& out_colors_all)
	{
		out_key_points_all.clear();
		out_descriptor_all.clear();
		Mat image;

		auto sift = xfeatures2d::SIFT::create();
		for (auto i_name = image_names.begin(); i_name != image_names.end(); ++i_name)
		{
			image = imread(*i_name);
			if (image.empty()) continue; // skip if no image

			vector<KeyPoint> key_points;
			Mat descriptor;
			sift->detectAndCompute(image, noArray(), key_points, descriptor);

			// ignore this image, if feature points is less
			if (key_points.size() <= 10) continue;

			out_key_points_all.push_back(key_points);
			out_descriptor_all.push_back(descriptor);

			// retrieve color value (RGB) from the image
			vector<Vec3b> colors(key_points.size());
			for (auto i = 0; i < key_points.size(); ++i)
			{
				auto p = key_points[i].pt;
				colors[i] = image.at<Vec3b>(p.y, p.x);
			}

			out_colors_all.push_back(colors);
		}
	}

	// match feature points
	void match_features(Mat& desc_query, Mat& desc_train, vector<DMatch>& out_matches)
	{
		vector<vector<DMatch>> knn_matches;
		BFMatcher matcher(NORM_L2);
		matcher.knnMatch(desc_query, desc_train, knn_matches, 2);

		// perform radio test
		auto min_dist = FLT_MAX;
		for (auto r = 0; r < knn_matches.size(); ++r)
		{
			if (knn_matches[r][0].distance > 0.6*knn_matches[r][1].distance) continue;
			auto dist = knn_matches[r][0].distance;
			if (dist < min_dist) min_dist = dist;
		}

		out_matches.clear();
		for (auto r = 0; r < knn_matches.size(); ++r)
		{
			// ignore matches that failed radio test and if the distance is too large
			if (knn_matches[r][0].distance > 0.6*knn_matches[r][1].distance ||
				knn_matches[r][0].distance > 5 * max(min_dist, 10.0f))
				continue;

			out_matches.push_back(knn_matches[r][0]);
		}
	}

	// extract the 2D point from the images based on matched features
	void get_matched_points(
		vector<KeyPoint>& key_points_query,
		vector<KeyPoint>& key_points_train,
		vector<DMatch> matches,
		vector<Point2f> out_p1,
		vector<Point2f> out_p2
	)
	{
		for (auto i = 0; i < matches.size(); i++)
		{
			out_p1.push_back(key_points_query[matches[i].queryIdx].pt);
			out_p2.push_back(key_points_query[matches[i].trainIdx].pt);
		}
	}

	void get_matched_colors(
		vector<Vec3b>& colors_query,
		vector<Vec3b>& colors_train,
		vector<DMatch> matches,
		vector<Vec3b> out_colors1,
		vector<Vec3b> out_colors2
	)
	{
		for (auto i = 0; i < matches.size(); i++)
		{
			out_colors1.push_back(colors_query[matches[i].queryIdx]);
			out_colors2.push_back(colors_train[matches[i].trainIdx]);
		}
	}

	// find R(rotation) & T(translation)
	bool find_transform(Mat& K, vector<Point2f>& p1, vector<Point2f>& p2, Mat& out_R, Mat& out_T, Mat& out_mask)
	{
		double focal_length = 0.5 * (K.at<double>(0) + K.at<double>(4));
		Point2d principle_point(K.at<double>(2), K.at<double>(5));

		Mat E = findEssentialMat(p1, p2, focal_length, principle_point, RANSAC, 0.99, 1, out_mask);
		if (E.empty()) return false;

		double feasible_count = countNonZero(out_mask);
		cout << (int)feasible_count << "-in-" << p1.size() << endl;
		// In RANSAC, if outlier > 50%, the result is most likely not accurate
		if (feasible_count <= 15 || (feasible_count / p1.size()) < .6) return false;

		// recover R(rotation) & T(Translation)
		int pass_count = recoverPose(E, p1, p2, out_R, out_T, focal_length, principle_point, out_mask);

		// number of passing inlier should more than 70%
		if ((double)pass_count / feasible_count < 0.7) return false;
		return true;
	}

	void maskout_points(vector<Point2f>& pts, Mat& mask)
	{
		vector<Point2f> pts_new;

		for (auto i = 0; i < mask.rows; i++)
		{
			if (mask.at<uchar>(i) == 1)
			{
				pts_new.push_back(pts[i]);
			}
		}

		pts = pts_new;
	}

	void maskout_colors(vector<Vec3b>& colors, Mat& mask)
	{
		vector<Vec3b> colors_new;

		for (auto i = 0; i < mask.rows; i++)
		{
			if (mask.at<uchar>(i) == 1)
			{
				colors_new.push_back(colors[i]);
			}
		}

		colors = colors_new;
	}

	// triangulation
	void reconstruct(
		Mat& K,
		Mat& R1,
		Mat& T1,
		Mat& R2,
		Mat& T2,
		vector<Point2f>& p1,
		vector<Point2f>& p2,
		vector<Point3f>& out_structure
	)
	{
		// cameras matrix
		Mat proj1(3, 4, CV_32FC1);
		Mat proj2(3, 4, CV_32FC1);

		R1.convertTo(proj2(Range(0, 3), Range(0, 3)), CV_32FC1);
		T1.convertTo(proj2.col(3), CV_32FC1);
		R2.convertTo(proj2(Range(0, 3), Range(0, 3)), CV_32FC1);
		T2.convertTo(proj2.col(3), CV_32FC1);

		Mat fK;
		K.convertTo(fK, CV_32FC1);
		proj1 = fK * proj1;
		proj2 = fK * proj2;

		// triangulation
		Mat structure_mat;
		triangulatePoints(proj1, proj2, p1, p2, structure_mat);

		// convert structure from Mat to vector<Point3f>
		out_structure.clear();
		out_structure.reserve(structure_mat.cols);
		for (auto i = 0; i < structure_mat.cols; ++i)
		{
			Mat_<float> col = structure_mat.col(i);
			col /= col(3);
			out_structure.push_back(Point3f(col(0), col(1), col(2)));
		}
	}

	void init_structure(
		Mat K,
		vector<vector<KeyPoint>>& key_points_all,
		vector<vector<Vec3b>>& colors_all,
		vector<vector<DMatch>>& matches_all,
		vector<Point3f>& out_structure,
		vector<vector<int>> out_correspond_struct_idx,
		vector<Vec3b>& out_colors,
		vector<Mat>& out_rotations,
		vector<Mat>& out_translations
	)
	{
		vector<Point2f> p1, p2;
		vector<Vec3b> c2;
		Mat R, T, mask;

		// filter out un-match feature points & colors
		get_matched_points(key_points_all[0], key_points_all[1], matches_all[0], p1, p2);
		get_matched_colors(colors_all[0], colors_all[1], matches_all[0], out_colors, c2);
		
		// find R & T
		find_transform(K, p1, p2, R, T, mask);

		// filter out points & colors with mask
		maskout_points(p1, mask);
		maskout_points(p2, mask);
		maskout_colors(out_colors, mask);

		// triangulation
		Mat R0 = Mat::eye(3, 3, CV_64FC1);
		Mat T0 = Mat::zeros(3, 1, CV_64FC1);
		reconstruct(K, R0, T0, R, T, p1, p2, out_structure);

		out_rotations = { R0, R };
		out_translations = { T0, T };

		// initilize correspond_struct_idx size as key_points_all
		out_correspond_struct_idx.clear();
		out_correspond_struct_idx.resize(key_points_all.size());
		for (auto i = 0; i < key_points_all.size(); ++i)
		{
			out_correspond_struct_idx[i].resize(key_points_all[i].size(), -1);
		}

		int index = 0;
		vector<DMatch>& matches = matches_all[0];
		for (auto i = 0; i < matches.size(); ++i)
		{
			if (mask.at<uchar>(i) == 0) continue;

			out_correspond_struct_idx[0][matches[i].queryIdx] = index;
			out_correspond_struct_idx[1][matches[i].trainIdx] = index;
			++index;
		}
	}
}

#endif // !SFM_H

