#pragma once
#pragma comment(lib, "gsl_point_id.lib")

struct gsl_touch_info
{
	int x[10];
	int y[10];
	int id[10];
	int finger_num;
};

#define	GESTURE_SIZE_NUM		32
typedef struct
{
	int coe;
	int out;
	unsigned int coor[GESTURE_SIZE_NUM / 2];
}GESTURE_MODEL_TYPE;

extern void gsl_ReportPressure(unsigned int *p);
extern int  gsl_TouchNear(void);
extern void gsl_DataInit(unsigned int * conf_in);
extern unsigned int gsl_version_id(void);
extern unsigned int gsl_mask_tiaoping(void);
extern void gsl_alg_id_main(struct gsl_touch_info *cinfo);
extern int gsl_obtain_gesture(void);
extern void gsl_GestureExtern(const GESTURE_MODEL_TYPE *model, int len);
extern unsigned int gsl_GestureBuffer(unsigned int **buf);