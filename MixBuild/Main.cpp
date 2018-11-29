#include <Windows.h>
#include <ShlObj.h>
#include <GL/glut.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <fstream>
#include "rc.h"
#include "viewer.h"

using namespace std;

viewer::Window __window;
viewer::Frustum __frustum;
viewer::WorldTransform __world;
viewer::TransformController __controller;

rc::PointCloud reconstruct_point_cloud(const String image_path, Size& out_image_size);
void generate_result_status(const bool status, const string result_path, const string output_path);
Point3f map_point_coordinate(Point3d point, Size image_size);
void render_model(int argc, char** argv, function<void()> draw_callback);
void __init_perspective_view(int width, int height);
void __init_lighting();
void __display();
void __reshape(int w, int h);
void __mouse(int button, int state, int x, int y);
void __motion(int x, int y);

int main(int argc, char* argv[])
{
	String image_path;
	//FreeConsole();
	//if (argc > 0)
	//{
	//	// only for debug purpose
	//	image_path = String(argv[1]);
	//}
	//else
	//{
	//	CHAR folder[MAX_PATH];
	//	HRESULT result = SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, folder);
	//	image_path = String(String(folder) + "\\MixBuild");
	//}

	CHAR folder[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, folder);
	image_path = String(String(folder) + "\\MixBuild");

	Size image_size;
	auto point_cloud = reconstruct_point_cloud(image_path, image_size);

	//generate_result_status(true, string(image_path + "\\model.txt"), image_path);

	auto draw_callback = [&]()
	{
		glBegin(GL_QUADS);
		glColor3d(1, 1, 1);

		for (const auto p : point_cloud)
		{
			Point3f mapped_point = map_point_coordinate(p, image_size);
			glVertex3f(mapped_point.x, mapped_point.y, mapped_point.z);
		}

		glEnd();
	};

	render_model(argc, argv, draw_callback);

	waitKey();

	return 0;
}

// reconstuct point cloud
rc::PointCloud reconstruct_point_cloud(const String image_path, Size& out_image_size)
{
	rc::ImageSrcSet image_src_set;
	try { rc::extract_image_src_set(image_path, image_src_set); }
	catch (const exception&) { return rc::PointCloud(); }

	rc::ShapeSet shape_set;
	rc::extract_shape(image_src_set, shape_set);

	rc::OthProjection oth_proj;
	rc::create_othogonal_projection(shape_set, oth_proj);
	out_image_size = oth_proj.front.size();

	rc::PointCloud point_cloud;
	int cube_size = 10;
	rc::calculate_point_cloud(oth_proj, point_cloud, cube_size);

	return point_cloud;
}

// generate the status json file for GUI
void generate_result_status(const bool status, const string result_path, const string output_path)
{
	rapidjson::Document document;
	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
	rapidjson::Value root(rapidjson::kObjectType);
	root.AddMember("status", true, allocator);
	root.AddMember("path", rapidjson::Value(result_path.c_str(), allocator), allocator);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	root.Accept(writer);

	ofstream ofs(string(output_path + "\\status.json"));
	ofs << buffer.GetString();
	ofs.close();
}

// map point cloud to volume form
Point3f map_point_coordinate(Point3d point, Size image_size)
{
	return Point3f(
		image_size.width / __window.width * point.x / __window.width,
		image_size.width / __window.width * point.y / __window.width,
		image_size.width / __window.width * point.z / __window.width
	);
}

// init opengl
void render_model(int argc, char** argv, function<void()> draw_callback)
{
	__window = {
		"Result",
		700,
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
	//__init_lighting();

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

// init lighting
void __init_lighting()
{
	float diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float specref[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float position[] = { 10.0f, 10.0f, 10.0f, 1.0f };
	short shininess = 255;

	glDisable(GL_LIGHTING);

	// create light
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glEnable(GL_LIGHT0);

	// create materials
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specref);
	glMateriali(GL_FRONT, GL_SHININESS, shininess);

	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);
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

	case 3:
	case 4:
		__world.translate(0, 0, button == 3 ? .1 : -.1);
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