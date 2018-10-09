#include "sfm.h"
#include <opencv2/viz.hpp>

using namespace std;

int main()
{
	vector<String> image_names;
	glob("./custom/", image_names);
	vector<vector<KeyPoint>> key_points_all;
	vector<Mat> descriptor_all;
	vector<vector<Vec3b>> colors_all;

	for (auto i = 0; i < image_names.size(); i++)
	{
		cout << image_names[i] << endl;
	}

	sfm::extract_features(image_names, key_points_all, descriptor_all, colors_all);

	for (auto i = 0; i < key_points_all.size(); i++)
	{
		cout << "key point" << i << ": " << key_points_all[i].size() << endl;
		cout << "color" << i << ": " << colors_all[i].size() << endl;
	}

	// perform feature points matching
	vector<vector<DMatch>> matches_all;
	matches_all.clear();
	for (auto i = 0; i < key_points_all.size() - 1; i++)
	{
		vector<DMatch> matches;
		sfm::match_features(descriptor_all[i], descriptor_all[i + 1], matches);
		matches_all.push_back(matches);
	}

	auto img = imread(image_names[0], IMREAD_COLOR);

	int focal = 800;
	Matx33d K(
		focal, 0, img.cols / 2,
		0, focal, img.rows / 2,
		0, 0, 1);
	Mat mat_k = Mat(K);

	cout << mat_k << endl;

	vector<Point3f> structure;
	vector<vector<int>> correspond_struct_idx;
	vector<Vec3b> colors;
	vector<Mat> rotations, translations;

	sfm::init_structure(
		mat_k,
		key_points_all,
		colors_all,
		matches_all,
		structure,
		correspond_struct_idx,
		colors,
		rotations,
		translations);

	for (auto i = 1; i < matches_all.size(); ++i)
	{
		vector<Point3f> object_points;
		vector<Point2f> image_points;
		Mat r, R, T;

		sfm::get_objpoints_and_imgpoints(
			matches_all[i],
			correspond_struct_idx[i],
			structure,
			key_points_all[i + 1],
			object_points,
			image_points
		);

		// if the matched points is less than 4, to prevent error occur in solvePnPRansac 
		if (object_points.size() < 4) continue;

		solvePnPRansac(object_points, image_points, K, noArray(), r, T);
		Rodrigues(r, R);
		rotations.push_back(R);
		translations.push_back(T);

		vector<Point2f> p1, p2;
		vector<Vec3b> colors1, colors2;
		sfm::get_matched_points(key_points_all[i], key_points_all[i + 1], matches_all[i], p1, p2);
		sfm::get_matched_colors(colors_all[i], colors_all[i + 1], matches_all[i], colors1, colors2);

		vector<Point3f> next_structure;
		sfm::reconstruct(mat_k, rotations[i], translations[i], R, T, p1, p2, next_structure);

		sfm::merge_structure(
			matches_all[i],
			correspond_struct_idx[i],
			correspond_struct_idx[i + 1],
			structure,
			next_structure,
			colors,
			colors1);
	}

	/*for (auto i = 0; i < structure.size(); i++)
	{

	}*/

	cv::viz::Viz3d visualizeWindow("3D");

	cv::viz::WCloud cloud = cv::viz::WCloud(structure, colors);
	cloud.setRenderingProperty(viz::POINT_SIZE, 2);

	visualizeWindow.showWidget("CLOUD", cloud);

	visualizeWindow.spin();
	return 0;
}