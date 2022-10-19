#ifndef PAINTERENGINE_APPLICATION_H
#define PAINTERENGINE_APPLICATION_H

#include "PainterEngine_Startup.h"
 
	typedef struct
	{
		PX_Runtime runtime;
		PX_FontModule fm;
		PX_Object* root, * button_start;
		PX_Object* sliderbar_size, * label_size;
		PX_Object* sliderbar_interval, * label_interval;
		PX_Object* sliderbar_layer, * label_layer;
		PX_Object* sliderbar_gravity, * label_gravity;
		PX_Object* sliderbar_spring, * label_spring;
		PX_Object* sliderbar_damp, * label_damp;
		PX_Object* sliderbar_gDamp, * label_gDamp;
		PX_Object* sliderbar_bDamp, * label_bDamp;
		PX_Object* sliderbar_shear, * label_shear;
		PX_Object* sliderbar_attract, * label_attract;
		PX_Object* sliderbar_velInit, * label_velInit;
		px_int value_size;
		px_int value_interval;
		px_int value_layer;
		px_int value_gravity;
		px_int value_spring;
		px_int value_damp;
		px_int value_gDamp;
		px_int value_bDamp;
		px_int value_shear;
		px_int value_attract;
		px_int value_velInit;
		 
		px_float* pos;
		px_float* vel;
		px_float* pos_sorted;
		px_float* vel_sorted;
		px_uint* ballHash;
		px_uint* ballIndex;
		px_uint* gridHash;
		px_float ballRadius;
		px_float ballDiameter;
		px_float ballInterval; 
		px_float BALL_POS_X_START;
		px_float BALL_POS_Y_START;
		px_int gridNum;
		int numThreads;
		px_uint* cellStart;
		px_uint* cellEnd;
		px_int BALL_NUMBER_IN_ROW;
		px_int BALL_NUMBER_IN_COL;
		px_int BALL_NUMBER;
		px_int BALL_NUMBER_INLAYER;
		px_float* renderDist;
		px_uint* renderSort;

	}PX_Application;

	extern PX_Application App;

	px_bool PX_ApplicationInitialize(PX_Application* App, px_int screen_Width, px_int screen_Height);
	px_void PX_ApplicationUpdate(PX_Application* App, px_dword elpased);
	px_void PX_ApplicationRender(PX_Application* App, px_dword elpased);
	px_void PX_ApplicationPostEvent(PX_Application* App, PX_Object_Event e);
 
#endif
