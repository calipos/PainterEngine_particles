#include "PainterEngine_Application.h"
#include <math.h>
#include "../kernel/PX_Object.h"
#include "sort.h" 
PX_Application App;

px_surface _3drenderTarget;
PX_3D_RenderList renderlist;
PX_3D_Camera camera;
PX_3D_World world;
PX_3D_ObjectData Obj;
PX_IO_Data io;
px_texture texture;
double time = 0;

px_point2D mouseStart;
px_point cameraPosition;
px_matrix  worldR, worldROld, Rt;
unsigned char colors[72] = {
	116,63,41,
	253,172,155,
	69,109,184,
	51,87,23,
	133,117,205,
	18,208,200,
	254,140,6,
	51,50,200,
	252,82,97,
	74,24,97,
	151,221,1,
	255,182,2,
	16,15,171,
	2,167,45,
	223,52,32,
	253,240,3,
	247,73,170,
	0,119,199,
	255,255,255,
	241,240,246,
	188,186,197,
	115,113,126,
	53,51,64,
	14,11,18,
};
float _QuatD(float w, float h)
{
	return  (abs(w) < abs(h) ? abs(w) : abs(h)) - 4;
}
float _QuatIX(float x, float w, float h)
{
	return (2.0f * x - w - 1.0f) / _QuatD(w, h);
}
float _QuatIY(float y, float w, float h)
{
	return (-2.0f * y + h - 1.0f) / _QuatD(w, h);
}
 
void trackball(
	const float w,
	const float h,
	const float speed_factor,
	const float down_mouse_x,
	const float down_mouse_y,
	const float mouse_x,
	const float mouse_y,
	px_point* axis,
	float* angle)
{
	float original_x = _QuatIX(speed_factor * (down_mouse_x - w / 2) + w / 2, w, h);
	float original_y = _QuatIY(speed_factor * (down_mouse_y - h / 2) + h / 2, w, h);

	float x = _QuatIX(speed_factor * (mouse_x - w / 2) + w / 2, w, h);
	float y = _QuatIY(speed_factor * (mouse_y - h / 2) + h / 2, w, h);

	float z = 1;
	float n0 = sqrt(original_x * original_x + original_y * original_y + z * z);
	float n1 = sqrt(x * x + y * y + z * z);
	if (n0 > 1e-4 && n1 > 1e-4)
	{
		float v0[] = { original_x / n0, original_y / n0, z / n0 };
		float v1[] = { x / n1, y / n1, z / n1 }; 
		axis->x = v0[1] * v1[2] - v0[2] * v1[1];
		axis->y = v0[2] * v1[0] - v0[0] * v1[2];
		axis->z = v0[0] * v1[1] - v0[1] * v1[0]; 
		float sa = sqrt(axis->x * axis->x + axis->y * axis->y + axis->z * axis->z);
		axis->x /= sa;
		axis->y /= sa;
		axis->z /= sa;
		float ca = v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2];
		*angle = atan2(sa, ca);
		if (x * x + y * y > 1.0)
		{
			(*angle) *= 1.0 + 0.2f * (sqrt(x * x + y * y) - 1.0);
		}
	}
}
void AngleAxisToRotationMatrix(px_point axis, float theta, px_matrix* R)
{
	static const float kOne = 1.f;
	if (1)
	{
		const float wx = axis.x;
		const float wy = axis.y;
		const float wz = axis.z;
		const float costheta = cos(theta);
		const float sintheta = sin(theta);

		// clang-format off
		R->m[0][0] = costheta + wx * wx * (kOne - costheta);
		R->m[1][0] = wz * sintheta + wx * wy * (kOne - costheta);
		R->m[2][0] = -wy * sintheta + wx * wz * (kOne - costheta);
		R->m[0][1] = wx * wy * (kOne - costheta) - wz * sintheta;
		R->m[1][1] = costheta + wy * wy * (kOne - costheta);
		R->m[2][1] = wx * sintheta + wy * wz * (kOne - costheta);
		R->m[0][2] = wy * sintheta + wx * wz * (kOne - costheta);
		R->m[1][2] = -wx * sintheta + wy * wz * (kOne - costheta);
		R->m[2][2] = costheta + wz * wz * (kOne - costheta);
		// clang-format on
	}
	else {
		// Near zero, we switch to using the first order Taylor expansion.
		float angle_axis_x = axis.x * theta;
		float angle_axis_y = axis.y * theta;
		float angle_axis_z = axis.z * theta;
		R->m[0][0] = kOne;
		R->m[1][0] = angle_axis_z;
		R->m[2][0] = -angle_axis_y;
		R->m[0][1] = -angle_axis_z;
		R->m[1][1] = kOne;
		R->m[2][1] = angle_axis_x;
		R->m[0][2] = angle_axis_y;
		R->m[1][2] = -angle_axis_x;
		R->m[2][2] = kOne;
	}
}

px_void PX_APP_3D_PixelShader(px_surface* psurface, px_int x, px_int y,
	px_point position, 
	px_float u, px_float v, 
	px_point4D normal, 
	px_texture* pTexture, px_color color)
{
	px_float alpha;
	px_float cosv = PX_Point4DDot(PX_Point4DUnit(normal), PX_POINT4D(0, 0, 1));
	cosv = -cosv;
	if (cosv > -0.33)
	{
		alpha = 60+cosv * 180;
		PX_SurfaceDrawPixel(psurface, x, y, PX_COLOR(255, alpha, alpha, alpha));
	}
}
 
#define STR(x) #x   
#define STR2(x) STR(x:) 
#define RIGISTER_ITEM_EVENT_FUN(item) \
px_void sliderbar_##item(PX_Object* pObject, PX_Object_Event e, px_void* ptr){ \
	PX_Application* pApp = (PX_Application*)ptr; \
	px_float x, y; \
	px_float objx, objy, objWidth, objHeight; \
	objx = pObject->x; \
	objy = pObject->y; \
	objWidth = pObject->Width; \
	objHeight = pObject->Height; \
	x = PX_Object_Event_GetCursorX(e); \
	y = PX_Object_Event_GetCursorY(e); \
	if (PX_isXYInRegion(x, y, objx, objy, objWidth, objHeight))	{ \
		pApp->value_##item = PX_Object_SliderBarGetValue(pObject); \
		PX_Object_SliderBarSetValue(pObject, pApp->value_##item); \
		px_char str[16];  \
		sprintf(str,"%d",pApp->value_##item); \
		PX_Object_EditSetText(pApp->label_##item, STR2(item));  PX_Object_EditAppendText(pApp->label_##item, str);}}
 
#define RIGISTER_ITEM(item,min_,max_,x,y,w,h,defaultV) \
pApp->label_##item = PX_Object_EditCreate(&pApp->runtime.mp_ui, pApp->root, x,y,w,h, PX_NULL); \
PX_Object_EditSetText(pApp->label_##item, STR2(item)); \
PX_Object_EditAppendText(pApp->label_##item, STR2(defaultV)); \
PX_Object_EditSetBackgroundColor(pApp->label_##item, PX_COLOR_WHITE); \
PX_Object_EditSetBorderColor(pApp->label_##item, PX_COLOR_WHITE); \
PX_Object_EditSetTextColor(pApp->label_##item, PX_COLOR_BLACK); \
pApp->sliderbar_##item = PX_Object_SliderBarCreate(&pApp->runtime.mp_ui, pApp->root, x, y+20, 100, 20, PX_OBJECT_SLIDERBAR_TYPE_HORIZONTAL, PX_OBJECT_SLIDERBAR_STYLE_BOX); \
PX_Object_SliderBarSetRange(pApp->sliderbar_##item, min_,max_); \
PX_Object_SliderBarSetColor(pApp->sliderbar_##item, PX_COLOR(255, 255, 255, 255)); \
PX_Object_SliderBarSetValue(pApp->sliderbar_##item, defaultV); \
pApp->value_##item=defaultV; \
PX_ObjectRegisterEvent(pApp->sliderbar_##item, PX_OBJECT_EVENT_CURSORDRAG, sliderbar_##item, pApp); 

RIGISTER_ITEM_EVENT_FUN(size);
RIGISTER_ITEM_EVENT_FUN(interval);
RIGISTER_ITEM_EVENT_FUN(layer);
RIGISTER_ITEM_EVENT_FUN(gravity);
RIGISTER_ITEM_EVENT_FUN(spring);
RIGISTER_ITEM_EVENT_FUN(damp);
RIGISTER_ITEM_EVENT_FUN(gDamp);
RIGISTER_ITEM_EVENT_FUN(bDamp);
RIGISTER_ITEM_EVENT_FUN(shear);
RIGISTER_ITEM_EVENT_FUN(attract); 
RIGISTER_ITEM_EVENT_FUN(velInit);
px_void reset(PX_Application* pApp)
{
	pApp->ballRadius = 0.00390625 * (2 << (pApp->value_size - 1));
	pApp->ballDiameter = 2 * pApp->ballRadius;
	pApp->ballInterval = pApp->value_interval * pApp->ballDiameter;
	pApp->BALL_POS_X_START = -pApp->ballRadius * (int)((.5 - pApp->ballRadius) / pApp->ballDiameter) * 2;
	pApp->BALL_POS_Y_START = -pApp->ballRadius * (int)((.5 - pApp->ballRadius) / pApp->ballDiameter) * 2;
	pApp->gridNum = 2 << (17);
	pApp->BALL_NUMBER_IN_ROW = (int)((.5 - pApp->ballRadius) / pApp->ballInterval) + (int)((.5 - pApp->ballRadius) / pApp->ballInterval) + 1;
	pApp->BALL_NUMBER_IN_COL = (int)((.5 - pApp->ballRadius) / pApp->ballInterval) + (int)((.5 - pApp->ballRadius) / pApp->ballInterval) + 1;
	pApp->BALL_NUMBER = pApp->BALL_NUMBER_IN_ROW * pApp->BALL_NUMBER_IN_COL * pApp->value_layer;
	pApp->BALL_NUMBER_INLAYER = pApp->BALL_NUMBER_IN_ROW * pApp->BALL_NUMBER_IN_COL;
	pApp->numThreads = min(256, pApp->BALL_NUMBER);
	if (pApp->cellStart != NULL) free(pApp->cellStart);
	if (pApp->cellEnd != NULL) free(pApp->cellEnd);
	if (pApp->pos != NULL) free(pApp->pos);
	if (pApp->vel != NULL) free(pApp->vel);
	if (pApp->pos_sorted != NULL) free(pApp->pos_sorted);
	if (pApp->vel_sorted != NULL) free(pApp->vel_sorted);
	if (pApp->ballHash != NULL) free(pApp->ballHash);
	if (pApp->ballIndex != NULL) free(pApp->ballIndex);
	if (pApp->gridHash != NULL) free(pApp->gridHash);
	if (pApp->renderDist != NULL) free(pApp->renderDist);
	if (pApp->renderSort != NULL) free(pApp->renderSort);
	pApp->cellStart = malloc(pApp->gridNum * sizeof(px_uint));
	pApp->cellEnd = malloc(pApp->gridNum * sizeof(px_uint));
	pApp->pos = malloc(pApp->BALL_NUMBER * 3 * sizeof(px_float));
	pApp->vel = malloc(pApp->BALL_NUMBER * 3 * sizeof(px_float));
	pApp->pos_sorted = malloc(pApp->BALL_NUMBER * 3 * sizeof(px_float));
	pApp->vel_sorted = malloc(pApp->BALL_NUMBER * 3 * sizeof(px_float));
	pApp->ballHash = malloc(pApp->BALL_NUMBER * sizeof(px_uint));
	pApp->ballIndex = malloc(pApp->BALL_NUMBER * sizeof(px_uint));
	pApp->gridHash = malloc((pApp->numThreads + 1) * sizeof(px_uint));
	pApp->renderDist = malloc(pApp->BALL_NUMBER * sizeof(px_float));
	pApp->renderSort = malloc(pApp->BALL_NUMBER * sizeof(px_uint));
	for (px_uint index = 0; index < pApp->BALL_NUMBER; index++)
	{
		px_uint index3 = index * 3;
		pApp->pos[index3 + 2] = pApp->ballRadius + index / pApp->BALL_NUMBER_INLAYER * pApp->ballInterval;
		px_uint posXY = index % pApp->BALL_NUMBER_INLAYER;
		pApp->pos[index3] = pApp->BALL_POS_X_START + posXY % pApp->BALL_NUMBER_IN_ROW * pApp->ballInterval;
		pApp->pos[index3 + 1] = pApp->BALL_POS_Y_START + posXY / pApp->BALL_NUMBER_IN_ROW * pApp->ballInterval;
		px_float randF0 = 1.-rand() % 200 * 0.01f;//x vel
		px_float randF1 = 1.-rand() % 200 * 0.01f;//y vel
		px_float randF2 = rand() % 1000 * 0.001f;//z vel	  
		pApp->vel[index3] = randF0 * 0.01*pApp->value_velInit;
		pApp->vel[index3 + 1] = randF1 * 0.01*pApp->value_velInit;
		pApp->vel[index3 + 2] = randF2 * 0.02 * pApp->value_velInit;
	}
}
px_void startUp(PX_Object* pObject, PX_Object_Event e, px_void* ptr)
{
	PX_Application* pApp = (PX_Application*)ptr;
	px_float x, y;
	px_float objx, objy, objWidth, objHeight;
	objx = pObject->x;
	objy = pObject->y;
	objWidth = pObject->Width;
	objHeight = pObject->Height;

	x = PX_Object_Event_GetCursorX(e);
	y = PX_Object_Event_GetCursorY(e);
	if (PX_isXYInRegion(x, y, objx, objy, objWidth, objHeight))
	{
		printf("start\n");
		reset(pApp);
	}
	px_int offset; 
}
 
px_bool PX_ApplicationInitialize(PX_Application* pApp, px_int screen_width, px_int screen_height)
{
	PX_ApplicationInitializeDefault(&pApp->runtime, screen_width, screen_height);
	PX_SurfaceCreate(&pApp->runtime.mp_game, 600, 600, &_3drenderTarget);
	//PX_TextureCreate(&pApp->runtime.mp_game, &texture, 300, 300);
	PX_3D_ObjectDataInitialize(&pApp->runtime.mp_game, &Obj);

	pApp->root = PX_ObjectCreate(&pApp->runtime.mp_ui, PX_NULL, 0, 0, 0, 0, 0, 0);
	{
		pApp->button_start = PX_Object_PushButtonCreate(&pApp->runtime.mp_ui, pApp->root, 600, 10, 100, 50, "start", PX_NULL);
		PX_Object_PushButtonSetBackgroundColor(pApp->button_start, PX_COLOR(125, 255, 0, 0));
		PX_Object_PushButtonSetTextColor(pApp->button_start, PX_COLOR(255, 0, 0, 0));
		PX_ObjectRegisterEvent(pApp->button_start, PX_OBJECT_EVENT_CURSORUP, startUp, pApp);
	}
	RIGISTER_ITEM(size, 1, 5, 600, 70, 100, 20,1);
	RIGISTER_ITEM(interval, 1, 8, 600, 110, 100, 20,1);
	RIGISTER_ITEM(layer, 1, 10, 600, 150, 100, 20,1);
	RIGISTER_ITEM(gravity, 0, 10, 600, 190, 100, 20,3);
	RIGISTER_ITEM(spring, 0, 10, 600, 230, 100, 20,5);
	RIGISTER_ITEM(damp, 0, 10, 600, 270, 100, 20,1);
	RIGISTER_ITEM(gDamp, 0, 10, 600, 310, 100, 20,10);
	RIGISTER_ITEM(bDamp, 0, 10, 600, 350, 100, 20,5);
	RIGISTER_ITEM(shear, 0, 10, 600, 390, 100, 20,1);
	RIGISTER_ITEM(attract, 0, 10, 600, 430, 100, 20,0);
	RIGISTER_ITEM(velInit, 0, 10, 600, 470, 100, 20, 5);
	reset(pApp);
	{
		{
			PX_3D_ObjectData_v ov;
			ov.x = -.5;		ov.z = -.5;		ov.y = 1;			PX_VectorPushback(&Obj.v, &ov);
			ov.x = .5;		ov.z = -.5;		ov.y = 1;			PX_VectorPushback(&Obj.v, &ov);
			ov.x = .5;		ov.z = .5;		ov.y = 1;			PX_VectorPushback(&Obj.v, &ov);
			ov.x = -.5;		ov.z = .5;		ov.y = 1;			PX_VectorPushback(&Obj.v, &ov);
			ov.x = -.5;		ov.z = -.5;		ov.y = 0;			PX_VectorPushback(&Obj.v, &ov);
			ov.x = .5;		ov.z = -.5;		ov.y = 0;			PX_VectorPushback(&Obj.v, &ov);
			ov.x = .5;		ov.z = .5;		ov.y = 0;			PX_VectorPushback(&Obj.v, &ov);
			ov.x = -.5;		ov.z = .5;		ov.y = 0;			PX_VectorPushback(&Obj.v, &ov);
			PX_3D_ObjectData_vn ovn;
			ovn.x = 0; ovn.y = 0, ovn.z = 1;			PX_VectorPushback(&Obj.vn, &ovn);
			ovn.x = 0; ovn.y = 0, ovn.z = -1;			PX_VectorPushback(&Obj.vn, &ovn);
			ovn.x = 0; ovn.y = 1, ovn.z = 0;			PX_VectorPushback(&Obj.vn, &ovn);
			ovn.x = 0; ovn.y = -1, ovn.z = 0;			PX_VectorPushback(&Obj.vn, &ovn);
			ovn.x = 1; ovn.y = 0, ovn.z = 0;			PX_VectorPushback(&Obj.vn, &ovn);
			ovn.x = -1; ovn.y = 0, ovn.z = 0;			PX_VectorPushback(&Obj.vn, &ovn);
			PX_3D_ObjectDataFace FaceA;
			FaceA.mtlFileNameIndex = -1;
			FaceA.mtlNameIndex = -1;
			FaceA.v[0].vt = -1;
			FaceA.v[1].vt = -1;
			FaceA.v[2].vt = -1;
			FaceA.v[3].vt = -1;
			{
				FaceA.v[0].v = 1;
				FaceA.v[1].v = 2;
				FaceA.v[2].v = 3;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 4;
				FaceA.v[1].vn = 4;
				FaceA.v[2].vn = 4;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 1;
				FaceA.v[1].v = 3;
				FaceA.v[2].v = 4;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 4;
				FaceA.v[1].vn = 4;
				FaceA.v[2].vn = 4;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 6;
				FaceA.v[1].v = 5;
				FaceA.v[2].v = 7;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 3;
				FaceA.v[1].vn = 3;
				FaceA.v[2].vn = 3;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 7;
				FaceA.v[1].v = 5;
				FaceA.v[2].v = 8;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 3;
				FaceA.v[1].vn = 3;
				FaceA.v[2].vn = 3;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 1;
				FaceA.v[1].v = 4;
				FaceA.v[2].v = 8;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 1;
				FaceA.v[1].vn = 1;
				FaceA.v[2].vn = 1;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 1;
				FaceA.v[1].v = 8;
				FaceA.v[2].v = 5;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 1;
				FaceA.v[1].vn = 1;
				FaceA.v[2].vn = 1;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 2;
				FaceA.v[1].v = 7;
				FaceA.v[2].v = 3;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 2;
				FaceA.v[1].vn = 2;
				FaceA.v[2].vn = 2;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 2;
				FaceA.v[1].v = 6;
				FaceA.v[2].v = 7;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 2;
				FaceA.v[1].vn = 2;
				FaceA.v[2].vn = 2;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 1;
				FaceA.v[1].v = 5;
				FaceA.v[2].v = 6;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 5;
				FaceA.v[1].vn = 5;
				FaceA.v[2].vn = 5;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 1;
				FaceA.v[1].v = 6;
				FaceA.v[2].v = 2;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 5;
				FaceA.v[1].vn = 5;
				FaceA.v[2].vn = 5;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 4;
				FaceA.v[1].v = 3;
				FaceA.v[2].v = 7;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 6;
				FaceA.v[1].vn = 6;
				FaceA.v[2].vn = 6;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
			{
				FaceA.v[0].v = 4;
				FaceA.v[1].v = 7;
				FaceA.v[2].v = 8;
				FaceA.v[3].v = -1;
				FaceA.v[0].vn = 6;
				FaceA.v[1].vn = 6;
				FaceA.v[2].vn = 6;
				FaceA.v[3].vn = -1;
				PX_VectorPushback(&Obj.face, &FaceA);
			}
		}
		mouseStart.x = -1;	mouseStart.y = -1;
		PX_MatrixIdentity(&world.mat);
		PX_MatrixIdentity(&worldROld);
		PX_MatrixIdentity(&worldR);
		PX_3D_RenderListInitialize(&pApp->runtime.mp_game, &renderlist,
			//PX_3D_PRESENTMODE_POINT |
			//PX_3D_PRESENTMODE_LINE |
			PX_3D_PRESENTMODE_TEXTURE |
			PX_3D_PRESENTMODE_PURE,
			PX_3D_CULLMODE_NONE,
			PX_NULL);		
		PX_3D_RenderListSetPixelShader(&renderlist, PX_APP_3D_PixelShader);
		PX_3D_ObjectDataToRenderList(&Obj, &renderlist);
		PX_3D_CameraUVNInitialize(&pApp->runtime.mp_game, &camera, PX_POINT4D(1, 1, 0), PX_POINT4D(0, 0, 0.), 1, 999, 90, 600, 600);
	}
	return PX_TRUE;
}

px_void PX_ApplicationUpdate(PX_Application* pApp, px_dword elpased)
{
	float gravity = (pApp->value_gravity) * 0.0001;
	float attraction = (pApp->value_attract) * 0.0001;
	float spring = pApp->value_spring * 0.1;
	float damping = pApp->value_damp * 0.1;
	float gdamping = pApp->value_gDamp * 0.1;
	float bdamping = pApp->value_bDamp * -0.1;
	float shear = pApp->value_shear * 0.1;
	float deltaTime = 0.5;
	for (px_uint index = 0; index < pApp->BALL_NUMBER; index++)
	{
		px_uint index3 = 3 * index;
		float* pos_x = &pApp->pos[index3];
		float* pos_y = &pApp->pos[index3 + 1];
		float* pos_z = &pApp->pos[index3 + 2];
		float* vel_x = &pApp->vel[index3];
		float* vel_y = &pApp->vel[index3 + 1];
		float* vel_z = &pApp->vel[index3 + 2];

		*vel_z += (gravity * deltaTime);
		*vel_x *= gdamping;
		*vel_y *= gdamping;
		*vel_z *= gdamping;
		*pos_x += *vel_x * deltaTime;
		*pos_y += *vel_y * deltaTime;
		*pos_z += *vel_z * deltaTime;
		if (*pos_x > .5 - pApp->ballRadius)
		{
			*pos_x = .5 - pApp->ballRadius;
			*vel_x *= bdamping;
		}
		if (*pos_x < -.5 + pApp->ballRadius)
		{
			*pos_x = -.5 + pApp->ballRadius;
			*vel_x *= bdamping;
		}
		if (*pos_y > .5 - pApp->ballRadius)
		{
			*pos_y = .5 - pApp->ballRadius;
			*vel_y *= bdamping;
		}
		if (*pos_y < -.5 + pApp->ballRadius)
		{
			*pos_y = -.5 + pApp->ballRadius;
			*vel_y *= bdamping;
		}
		if (*pos_z > 1 - pApp->ballRadius)
		{
			*pos_z = 1 - pApp->ballRadius;
			*vel_z *= bdamping;
		}
		if (*pos_z < 0 + pApp->ballRadius)
		{
			*pos_z = 0 + pApp->ballRadius;
			*vel_z = abs(bdamping * *vel_z);
		}
	}
	sort(&pApp->BALL_NUMBER, pApp->pos, &pApp->ballDiameter, pApp->ballHash, pApp->ballIndex);	
	memset(pApp->cellStart, 0xff, pApp->gridNum * sizeof(px_uint));
	for (px_uint index = 0; index < pApp->BALL_NUMBER; index++)
	{
		px_uint threadIdx = index % pApp->numThreads;
		px_uint* hash = 0;
		if (threadIdx == 0)
		{
			for (px_uint index2 = 0; index2 < min(pApp->BALL_NUMBER - index, pApp->numThreads); index2++)
			{
				hash = &pApp->ballHash[index + index2];
				pApp->gridHash[index2 + 1] = *hash;
				if (index > 0 && threadIdx == 0)
				{
					pApp->gridHash[0] = pApp->ballHash[index - 1];
				}
			}
		}
		{
			hash = &pApp->ballHash[index];
			if (index == 0 || *hash != pApp->gridHash[threadIdx])
			{
				pApp->cellStart[*hash] = index;
				if (index > 0)
					pApp->cellEnd[pApp->gridHash[threadIdx]] = index;
			}
			if (index == pApp->BALL_NUMBER - 1)
			{
				pApp->cellEnd[*hash] = index + 1;
			}
			px_uint sortedIndex = pApp->ballIndex[index];
			px_uint sortedIndex3 = 3 * pApp->ballIndex[index];
			px_uint index3 = 3 * index;
			pApp->pos_sorted[index3] = pApp->pos[sortedIndex3];
			pApp->pos_sorted[index3 + 1] = pApp->pos[sortedIndex3 + 1];
			pApp->pos_sorted[index3 + 2] = pApp->pos[sortedIndex3 + 2];
			pApp->vel_sorted[index3 + 0] = pApp->vel[sortedIndex3];
			pApp->vel_sorted[index3 + 1] = pApp->vel[sortedIndex3 + 1];
			pApp->vel_sorted[index3 + 2] = pApp->vel[sortedIndex3 + 2];
		}
	}
	for (px_uint index = 0; index < pApp->BALL_NUMBER; index++)
	{ 
		px_uint index3 = 3 * index;
		float* pos_x = &pApp->pos_sorted[index3];
		float* pos_y = &pApp->pos_sorted[index3 + 1];
		float* pos_z = &pApp->pos_sorted[index3 + 2];
		float* vel_x = &pApp->vel_sorted[index3];
		float* vel_y = &pApp->vel_sorted[index3 + 1];
		float* vel_z = &pApp->vel_sorted[index3 + 2];
		int gridPos_x = floor((*pos_x +.5) / pApp->ballDiameter);
		int gridPos_y = floor((*pos_y +.5) / pApp->ballDiameter);
		int gridPos_z = floor((*pos_z) / pApp->ballDiameter);
		float force_x = 0.f;
		float force_y = 0.f;
		float force_z = 0.f;
		for (int z = -1; z <= 1; z++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int x = -1; x <= 1; x++)
				{
					int neighbourPos_x = gridPos_x + x;
					int neighbourPos_y = gridPos_y + y;
					int neighbourPos_z = gridPos_z + z;
					neighbourPos_x = neighbourPos_x & 63;
					neighbourPos_y = neighbourPos_y & 63;
					neighbourPos_z = neighbourPos_z & 63;
					unsigned int gridHash = (neighbourPos_z * 4096) + (neighbourPos_y * 64) + neighbourPos_x;
					unsigned int startIndex = pApp->cellStart[gridHash];
					if (startIndex != 0xffffffff)
					{
						unsigned int endIndex = pApp->cellEnd[gridHash];
						for (unsigned int j = startIndex; j < endIndex; j++)
						{
							if (j != index)
							{
								float* pos2 = &pApp->pos_sorted[3 * j];
								float* vel2 = &pApp->vel_sorted[3 * j];
								float force[3] = { 0,0,0 };
								{
									float relPos_x = *pos2 - *pos_x;
									float relPos_y = *(pos2 + 1) - *(pos_x + 1);
									float relPos_z = *(pos2 + 2) - *(pos_x + 2);
									float dist = sqrt(relPos_x * relPos_x + relPos_y * relPos_y + relPos_z * relPos_z); 
									float collideDist = pApp->ballDiameter;
									float* force_x = force;
									float* force_y = force + 1;
									float* force_z = force + 2;
									if (dist < collideDist)
									{
										*force_x = relPos_x * attraction;
										*force_y = relPos_y * attraction;
										*force_z = relPos_z * attraction;
										relPos_x /= dist;
										relPos_y /= dist;
										relPos_z /= dist;
										float spring_c = (collideDist - dist) * (-spring);
										*force_x += relPos_x * spring_c;
										*force_y += relPos_y * spring_c;
										*force_z += relPos_z * spring_c;
										float relVel_x = *vel2 - *vel_x;
										float relVel_y = *(vel2 + 1) - *(vel_x + 1);
										float relVel_z = *(vel2 + 2) - *(vel_x + 2);
										*force_x += relVel_x * damping;
										*force_y += relVel_y * damping;
										*force_z += relVel_z * damping;
										float dot = relVel_x * relPos_x + relVel_y * relPos_y + relVel_z * relPos_z;
										*force_x += shear * (relVel_x - dot * relPos_x);
										*force_y += shear * (relVel_y - dot * relPos_y);
										*force_z += shear * (relVel_z - dot * relPos_z);
									} 
								}
								force_x += force[0];
								force_y += force[1];
								force_z += force[2];
							}
						}
					}
				}
			}
		}
		unsigned int originalIndex3 = 3 * pApp->ballIndex[index];
		pApp->vel[originalIndex3] = *vel_x + force_x;
		pApp->vel[originalIndex3 + 1] = *vel_y + force_y;
		pApp->vel[originalIndex3 + 2] = *vel_z + force_z;
	}
}
px_void PX_ApplicationRender(PX_Application* pApp, px_dword elpased)
{
	px_surface* pRenderSurface = &pApp->runtime.RenderSurface;
	PX_RuntimeRenderClear(&pApp->runtime, PX_COLOR(255, 255, 255, 255));	
	{
		PX_SurfaceClear(&_3drenderTarget, 0, 0, _3drenderTarget.width - 1, _3drenderTarget.height - 1, PX_COLOR(255, 255, 255, 255));
		PX_3D_Scene(&renderlist, &world, &camera);
		PX_3D_Present(&_3drenderTarget, &renderlist, &camera);
		PX_SurfaceRender(pRenderSurface, &_3drenderTarget, 0, 0, PX_ALIGN_LEFTTOP, PX_NULL);
	}
	Rt = camera.mat_cam;
	Rt._41 = 0;	Rt._42 = 0;	Rt._43 = 0;	Rt._44 = 1.0f;
	px_point t0;
	t0.x = -camera.position.x;
	t0.y = -camera.position.y;
	t0.z = -camera.position.z;
	t0 = PX_PointMulMatrix(t0, Rt);
	Rt = PX_MatrixMultiply(worldROld, Rt);
	Rt._41 = t0.x;	Rt._42 = t0.y;	Rt._43 = t0.z;	Rt._44 = 1.0f;
	for (px_uint index = 0; index < pApp->BALL_NUMBER; index++)
	{
		int index3 = 3 * index;
		px_point t;
		t.x = pApp->pos[index3];
		t.y = 1-pApp->pos[index3 + 2];
		t.z = pApp->pos[index3 + 1];
		t = PX_PointMulMatrix(t, Rt);
		pApp->pos_sorted[index3] = t.x;
		pApp->pos_sorted[index3 + 1] = t.y;
		pApp->pos_sorted[index3 + 2] = t.z;
		pApp->renderDist[index] = t.z;
	}
	sortRender(&pApp->BALL_NUMBER, pApp->renderDist, pApp->renderSort);
	int drawRadius = 2;
	if (pApp->value_size == 3) drawRadius = 5;
	else if (pApp->value_size == 4) drawRadius = 11;
	else if (pApp->value_size == 5) drawRadius = 15;
	for (px_uint i = 0; i < pApp->BALL_NUMBER; i++)
	{
		int index = pApp->renderSort[i];
		int index3 = 3 * index;
		px_point t;
		t.x = pApp->pos_sorted[index3];
		t.y = pApp->pos_sorted[index3 + 1];
		t.z = pApp->pos_sorted[index3 + 2];
		float u = t.x / t.z;
		float v = t.y / t.z;
		u = 300 * u + camera.viewport_center_x;
		v = camera.viewport_center_y - 300 * v;
		int rgbId3 = 3 * (index % 24);
		PX_GeoDrawSolidCircle(pRenderSurface, u, v, drawRadius, PX_COLOR(255, colors[rgbId3], colors[rgbId3+1], colors[rgbId3+2]));
	}	
	PX_ObjectRender(pRenderSurface, pApp->root, elpased);	//
	//PX_TextureRender(pRenderSurface, &texture, pApp->runtime.surface_width / 2, pApp->runtime.window_height / 2, PX_ALIGN_CENTER, PX_NULL);
} 
px_void PX_ApplicationPostEvent(PX_Application* pApp, PX_Object_Event e)
{
	px_surface* pRenderSurface = &pApp->runtime.RenderSurface;
	PX_ApplicationEventDefault(&pApp->runtime, e);
	PX_ObjectPostEvent(pApp->root, e);

	if (e.Event == PX_OBJECT_EVENT_CURSORDRAG && mouseStart.x >= 0
		&& abs(mouseStart.x - e.Param_float[0]) >= 2
		&& abs(mouseStart.y - e.Param_float[1]) >= 2)
	{
		px_point axis;
		float angle;
		trackball(
			pRenderSurface->width, pRenderSurface->height,
			0.1,
			mouseStart.x, mouseStart.y,
			e.Param_float[0], e.Param_float[1],
			&axis, &angle);
		AngleAxisToRotationMatrix(axis, angle, &worldR);
		worldROld = PX_MatrixMultiply(worldROld, worldR);
		PX_MatrixIdentity(&world.mat);
		world.mat = PX_MatrixMultiply(world.mat, worldROld);
	}
	else if (e.Event == PX_OBJECT_EVENT_CURSORUP)
	{
		mouseStart.x = -1;	mouseStart.y = -1;
	}
	else if (e.Event == PX_OBJECT_EVENT_CURSORDOWN && mouseStart.x < 0)
	{
		mouseStart.x = e.Param_float[0];	mouseStart.y = e.Param_float[1];
	}
}

