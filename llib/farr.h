/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#ifndef __LLIB_FARR_H
#define __LLIB_FARR_H

// this marlarky forces constants like M_PI to be available
#define _USE_MATH_DEFINES
#define __USE_XOPEN
#include <math.h>

typedef double (*FarrMapFun)(double);

typedef double* farr_t;

#define farr_from_seq(s) ((double*)seq_array_ref(s))
#define farr_copy(arr) array_new_copy(double,arr,sizeof(arr)/sizeof(double))

double *farr_range(double x1, double x2, double dx);
double *farr_map(double *a, FarrMapFun f);
double *farr_sample(double *A, int i1, int i2, int istep);
double *farr_sample_float(float *A, int i1, int i2, int istep);
double *farr_sample_int(int *A, int i1, int i2, int istep);
void farr_scale(double *A, double m, double c);
double *farr_2(double x1,double x2);
double *farr_4(double x1,double x2,double x3,double x4);

#endif
