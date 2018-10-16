#include <opencv2/opencv.hpp>
#include <opencv2/viz.hpp>
#include "rc.h"

using namespace std;
using namespace cv;

int main()
{
	vector<String> image_names;
	glob("./imgs/", image_names);

	auto size = imread(image_names[0]).size();

	vector<rc::Contours> contours_collection;
	vector<rc::Corners> corners_collection;
	rc::PointCloud point_cloud;
	rc::extract_contours(image_names, contours_collection);
	rc::extract_corners(image_names, corners_collection);

	for (auto i = 0; i < image_names.size(); i++)
	{
		auto contours = contours_collection[i];
		auto corners = corners_collection[i];
		rc::PointCloud pc;
		rc::map_corners_to_point_cloud(corners, size.width, size.height, pc);

		rc::rotate_y_axis(pc, 45 * (i + 1));
		rc::merge_point_cloud(pc, point_cloud);

		Mat img_detected = Mat::zeros(size, CV_8UC3);
		/*for (auto i = 0; i < contours.size(); i++)
		{
			auto color = Scalar(255, 255, 255);
			drawContours(img_contours, contours, i, color, 2);
		}*/

		/*for (auto i = 0; i < corners.size(); i++)
		{
			auto color = Scalar(0, 255, 0);
			circle(img_detected, corners[i], 4, color, 4, FILLED);
		}*/

		/*resize(img_detected, img_detected, img_detected.size() / 2);
		imshow(image_names[i], img_detected);*/
	}

	viz::Viz3d window("Coordinate Frame");
	viz::WCloud cloud_widget(point_cloud);
	cloud_widget.setRenderingProperty(viz::POINT_SIZE, 3);
	window.showWidget("plc", cloud_widget);

	window.spin();

	waitKey();

	return 0;
}