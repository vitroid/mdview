#include <stdio.h>
#include <math.h>
#include "mdview.h"
#define TWO_PI (2.0*3.14159265358979323)
#define TREMBLE_RADIUS (2.0)
#define TREMBLE_ANGLE (30)

static void TrembleAngle (double *p1, double *p2, double *p3);
static unsigned int tremble_status = 0;
static int tremble_angle = 0;
static double tremble_theta, tremble_psi, tremble_phi;

extern unsigned int TrembleStatus (void) {
	return tremble_status;
}

extern void TrembleToggle (void) {
	if (tremble_status) {
		SetEulerAngle(tremble_theta, tremble_psi, tremble_phi);
		UpdateViewData();
		DrawObject();
	}
	tremble_status ^= 1;
	return;
}

extern void TrembleSaveEulerAngle(void) {
	GetEulerAngle(&tremble_theta, &tremble_psi, &tremble_phi);
	return;
}

extern void TrembleLoadEulerAngle(void) {
	SetEulerAngle(tremble_theta, tremble_psi, tremble_phi);
	return;
}

static void TrembleAngle (double *p1, double *p2, double *p3) {
	double rx,ry,rz,r,t,coef,dx,dy;
	int win_w, win_h;
	tremble_angle += TREMBLE_ANGLE;
	tremble_angle = tremble_angle % 360;
	GetScreenSize(&win_w, &win_h);
	coef = ((win_w < win_h)? win_w: win_h) * GetZoomPercent();
	dx = TREMBLE_RADIUS*cos(tremble_angle / 360.0 * TWO_PI);
	dy = TREMBLE_RADIUS*sin(tremble_angle / 360.0 * TWO_PI);
	rx = TWO_PI*(dx)/coef;
	ry = TWO_PI*(dy)/coef;
	r = sqrt(rx*rx + ry*ry);
	t = rx / r;
	if (t > 1.0)       rz = 0.0;
	else if (t < -1.0) rz = TWO_PI/2.0;
	else               rz = acos(t);
	if (ry < 0) rz = -rz;
	*p1 = (-rz);
	*p2 = -r;
	*p3 = rz;
	return;
}

extern void TrembleRotateMatrix (void) {
	double tremble_1, tremble_2, tremble_3;
	TrembleAngle(&tremble_1, &tremble_2, &tremble_3);
	RotateMatrixZ(tremble_1);
	RotateMatrixY(tremble_2);
	RotateMatrixZ(tremble_3);
	return;
}
