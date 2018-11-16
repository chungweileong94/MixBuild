#include <Windows.h>
#include <opencv2/opencv.hpp>
#include <opencv2/viz.hpp>
#include "rc.h"

using namespace std;
using namespace cv;

int main(int argc, char* argv[])
{
	//FreeConsole();
	if (argc < 1) return -1;

	String image_path(argv[1]);

	rc::ImageSrcSet image_src_set;
	try { rc::extract_image_src_set(image_path, image_src_set); }
	catch (const std::exception&) { return -2; }

	rc::ShapeSet shape_set;
	rc::extract_shape(image_src_set, shape_set);

	rc::OthProjection oth_proj;
	rc::create_othogonal_projection(shape_set, oth_proj);

	rc::PointCloud point_cloud;
	int cube_size = 10;
	rc::calculate_point_cloud(oth_proj, point_cloud, cube_size);

	viz::Viz3d window("Coordinate Frame");
	viz::WCloud cloud_widget(point_cloud);
	cloud_widget.setRenderingProperty(viz::POINT_SIZE, 2);
	window.showWidget("plc", cloud_widget);

	// show guideline point
	auto size = oth_proj.front.size();
	rc::PointCloud ddd_guide_point{
		Point3f(0, 0, 0),
		Point3f(-size.width / 2, 0, -size.height / 2),
		Point3f(size.width / 2, 0, -size.height / 2),
		Point3f(-size.width / 2, 0, size.height / 2),
		Point3f(size.width / 2, 0, size.height / 2),
	};
	viz::WCloud ddd_guide(ddd_guide_point, viz::Color::lime());
	ddd_guide.setRenderingProperty(viz::POINT_SIZE, 5);
	window.showWidget("ddd", ddd_guide);

	/*rc::PointCloud dd_guide_point{
		Point3f(0, 0, 0),
		Point3f(0, 0, size.height),
		Point3f(size.width, 0, 0),
		Point3f(size.width, 0, size.height),
		Point3f(size.width / 2, 0, size.height / 2),
	};
	viz::WCloud dd_guide(dd_guide_point, viz::Color::red());
	dd_guide.setRenderingProperty(viz::POINT_SIZE, 5);
	window.showWidget("dd", dd_guide);*/

	window.spin();

	waitKey();

	return 0;
}