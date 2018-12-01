#pragma once

#ifndef VIEWER_H
#define VIEWER_H

#include <string>

using namespace std;

namespace viewer
{
	struct Window
	{
		string title;
		int width, height;
		function<void()> draw_callback;
	};

	struct Frustum
	{
		double eye_x = 0, eye_y = 0, eye_z = 40;
		double ref_x = 0, ref_y = 0, ref_z = 0;
		double up_x = 0, up_y = 1, up_z = 0;
		double near_z = .1, far_z = 500;
		double field_of_view = 45; //Angle in Y direction
		double aspect;
	};

	struct WorldTransform
	{
		float translate_x = 0, translate_y = 0, translate_z = 30;
		float rotate_x = 20, rotate_y = 20, rotate_z = 0;
		float scale_x = 1, scale_y = 1, scale_z = 1;

		void translate(float x, float y, float z)
		{
			translate_x += x;
			translate_y += y;
			translate_z += z;
		}

		void rotate(float x, float y, float z)
		{
			rotate_x += x;
			rotate_y += y;
			rotate_z += z;
		}
	};

	struct TransformController
	{
		double display_amt = .5;
		double rotate_amt = 2;
		int mouse_x, mouse_y;
		bool left_mouse_is_pressed, right_mouse_is_pressed;
	};
}

#endif // !VIEWER_H
