#include <Windows.h>
#include <ShlObj.h>
#include <GL/glut.h>
#include <opencv2/viz.hpp>
#include "rc.h"
#include "viewer.h"

using namespace std;

viewer::Window __window;
viewer::Frustum __frustum;
viewer::WorldTransform __world;
viewer::TransformController __controller;

rc::PointCloud reconstruct_point_cloud(const String image_path);
void viz_display(rc::PointCloud point_cloud, Size guide_size);
void render_model(int argc, char** argv, function<void()> draw_callback);
void __init_perspective_view(int width, int height);
void __display();
void __reshape(int w, int h);
void __mouse(int button, int state, int x, int y);
void __motion(int x, int y);

int main(int argc, char* argv[])
{
	String image_path;
	//FreeConsole();
	if (argc > 0)
	{
		// only for debug purpose
		image_path = String(argv[1]);
	}
	else
	{
		CHAR folder[MAX_PATH];
		HRESULT result = SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, folder);
		image_path = String(String(folder) + "\\MixBuild");
	}

	auto point_cloud = reconstruct_point_cloud(image_path);
	//viz_display(point_cloud, Size(1920, 1080));

	auto draw_callback = [&]()
	{
		//glutWireTeapot(1);

		glBegin(GL_POINTS);

		for (const auto p : point_cloud)
		{
			float x = 1920 / 1000 * p.x / 1000;
			float y = 1080 / 700 * p.y / 700;
			float z = 1080 / 700 * p.z / 700;

			glPointSize(20);
			glVertex3f(x, y, z);
		}

		glEnd();
	};

	render_model(argc, argv, draw_callback);

	waitKey();

	return 0;
}

rc::PointCloud reconstruct_point_cloud(const String image_path)
{
	rc::ImageSrcSet image_src_set;
	try { rc::extract_image_src_set(image_path, image_src_set); }
	catch (const exception&) { return rc::PointCloud(); }

	rc::ShapeSet shape_set;
	rc::extract_shape(image_src_set, shape_set);

	rc::OthProjection oth_proj;
	rc::create_othogonal_projection(shape_set, oth_proj);

	rc::PointCloud point_cloud;
	int cube_size = 10;
	rc::calculate_point_cloud(oth_proj, point_cloud, cube_size);

	return point_cloud;
}

void viz_display(rc::PointCloud point_cloud, Size guide_size)
{
	viz::Viz3d window("Coordinate Frame");
	viz::WCloud cloud_widget(point_cloud);
	cloud_widget.setRenderingProperty(viz::POINT_SIZE, 2);
	window.showWidget("plc", cloud_widget);

	// show guideline point
	auto size = guide_size;
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
}

// init opengl
void render_model(int argc, char** argv, function<void()> draw_callback)
{
	__window = {
		"Result",
		1000,
		700,
		draw_callback
	};

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowSize(__window.width, __window.height);
	glutCreateWindow(__window.title.c_str());

	glutDisplayFunc(__display);
	glutReshapeFunc(__reshape);

	glutMouseFunc(__mouse);
	glutMotionFunc(__motion);

	glEnable(GL_DEPTH_TEST);
	glClearColor(.2, .2, .2, 1);

	__init_perspective_view(__window.width, __window.height);

	glutMainLoop();
}

// init perspetive
void __init_perspective_view(int width, int height)
{
	__frustum.aspect = width / height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(__frustum.field_of_view, __frustum.aspect, __frustum.near_z, __frustum.far_z);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(
		__frustum.eye_x, __frustum.eye_y, __frustum.eye_z,
		__frustum.ref_x, __frustum.ref_y, __frustum.ref_z,
		__frustum.up_x, __frustum.up_y, __frustum.up_z
	);
}

// window display callback
void __display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	glTranslatef(__world.translate_x, __world.translate_y, __world.translate_z);
	glRotatef(__world.rotate_x, 1.0f, 0.0f, 0.0f);
	glRotatef(__world.rotate_y, 0.0f, 1.0f, 0.0f);
	glRotatef(__world.rotate_z, 0.0f, 0.0f, 1.0f);
	glScalef(__world.scale_x, __world.scale_y, __world.scale_z);

	glColor3d(1, 1, 1);
	glPushMatrix();
	__window.draw_callback();
	glPopMatrix();

	glPopMatrix();

	glFlush();
	glutSwapBuffers();
	glutPostRedisplay();
}

// window reshape
void __reshape(int w, int h)
{
	glutReshapeWindow(__window.width, __window.height);
}

// mouse click callback
void __mouse(int button, int state, int x, int y)
{
	y = __window.height - y;

	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN && !__controller.left_mouse_is_pressed)
		{
			__controller.mouse_x = x;
			__controller.mouse_y = y;
			__controller.left_mouse_is_pressed = true;
		}

		if (state == GLUT_UP && __controller.left_mouse_is_pressed)
		{
			__controller.left_mouse_is_pressed = false;
		}
		break;
	}
}

// mouse montion callback
void __motion(int x, int y)
{
	y = __window.height - y;
	int dx = x - __controller.mouse_x;
	int dy = y - __controller.mouse_y;

	if (__controller.left_mouse_is_pressed)
	{
		__world.rotate(-dy * 0.2f, 0, 0);
		__world.rotate(0, dx * 0.2f, 0);
	}

	__controller.mouse_x = x;
	__controller.mouse_y = y;
	glutPostRedisplay();
}