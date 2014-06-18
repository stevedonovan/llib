/*
* llib little C library
* BSD licence
* Copyright Steve Donovan, 2013
*/

#include "obj.h"
#include "farr.h"

///
// ### Functions operating on arrays of floating-point values.
//
// `farr_sample_int` and `farr_sample_float` can be used to convert
// arrays of ints and floats to arrays of doubles.
// @module farr

/// Sampling Arrays.
// @section sample

static farr_t sample_helper(void *A, int i1, int istep, int *pi2) {
    if (*pi2 == -1)
        *pi2 = array_len(A);
    int n = (*pi2 - i1)/istep;
    return array_new(double,n);    
}

/// sample an array of doubles.
double *farr_sample(double *A, int i1, int i2, int istep) {
    farr_t res = sample_helper(A,i1,istep,&i2);
    for (int i = i1, k = 0; i < i2; i += istep,k++)
        res[k] = A[i];
    return res;
}

/// sample an array of floats
double *farr_sample_float(float *A, int i1, int i2, int istep) {
    farr_t res = sample_helper(A,i1,istep,&i2);
    for (int i = i1, k = 0; i < i2; i += istep,k++)
        res[k] = A[i];
    return res;
}

/// sample an array of ints
double *farr_sample_int(int *A, int i1, int i2, int istep) {
    farr_t res = sample_helper(A,i1,istep,&i2);
    for (int i = i1, k = 0; i < i2; i += istep,k++)
        res[k] = A[i];
    return res;
}

/// Creating and Transforming
// @section create

/// an array generated between `x1` and `x2`, with delta `dx`.
double *farr_range(double x1, double x2, double dx) {
    int n = ceil((x2 - x1)/dx);
    double *arr = array_new(double,n);
    int i = 0;
    for (double x = x1; x < x2; x += dx) {
        arr[i++] = x;
    }
    return arr;
}

/// create a new array by applying a function to each value of  an array.
double *farr_map(double *a, FarrMapFun f) {
    int n = array_len(a);
    double *b = array_new(double,n);
    FOR(i,n) {
        b[i] = f(a[i]);
    }
    return b;
}

/// transform an array by multipying its values by `m` and adding `c`.
void farr_scale(double *A, double m, double c) {
    FOR(i,array_len(A))
        A[i] = m*A[i] + c;
}

/// array of doubles from a sequence
// @function farr_from_seq

/// create dynanic array from a statically declared array of doubles.
// @function farr_copy

/// double array of two elements.
double *farr_2(double x1,double x2) {
    double *res = array_new(double,2);
    res[0] = x1;
    res[1] = x2;
    return res;
}

/// double array of four elements.
double *farr_4(double x1,double x2,double x3,double x4) {
    double *res = array_new(double,4);
    res[0] = x1;
    res[1] = x2;
    res[2] = x3;
    res[3] = x4;
    return res;
}


