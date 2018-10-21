#include <opencv2/opencv.hpp>
#include <opencv2/viz.hpp>
#include "rc.h"

using namespace std;
using namespace cv;

int main()
{
	rc::ImageSet image_set;
	rc::extract_image_set("./imgs/", image_set);

	auto size = imread(image_set[0]).size();

	rc::CornersSet corners_set;
	rc::PointCloud point_cloud;
	rc::extract_corners(image_set, corners_set);

	// 90 - 360
	for (auto i = 0; i < 360; i += 90)
	{
		int ref_left = i - 90 < 0 ? i - 90 + 360 : i - 90;
		int ref_right = i + 90 >= 360 ? i + 90 - 360 : i + 90;

		auto corners = corners_set[i];
		auto corners_ref1 = corners_set[ref_left];
		auto corners_ref2 = corners_set[ref_right];
		auto depth = (rc::get_corners_width(corners_ref1) + rc::get_corners_width(corners_ref2)) / 2 / 2;

		rc::PointCloud pc;
		rc::map_corners_to_point_cloud(corners, size.width, size.height, depth, pc);

		rc::rotate_y_axis(pc, -45 * i);
		rc::merge_point_cloud(pc, point_cloud);
	}

	viz::Viz3d window("Coordinate Frame");
	viz::WCloud cloud_widget(point_cloud);
	cloud_widget.setRenderingProperty(viz::POINT_SIZE, 2);
	window.showWidget("plc", cloud_widget);

	window.spin();

	waitKey();

	return 0;
}




//Mat img_detected = Mat::zeros(size, CV_8UC3);
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