/*
vapor.h header made by @deadpitixd,

If you make changes, run the vp_test.c file and check if everything passes.
Only commit if everything is successful.
*/

#pragma once
#ifndef __VAPOR_H
#define __VAPOR_H

//#define __VP_DISABLE_ERRORLOG

// Neccesary include for some data types
#include <stdint.h>
// String
#include <string.h>
// Input output
#include <stdio.h>
// Ansi escape codes
#include "vapor_colors.h"
// Color macros
#include <math.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>

#define vp_EPSILON 0.00001f
#define vp_PI 3.14159265358979323846

#define vp_rgb3i(a) (a).r, (a).g, (a).b
#define vp_rgb3normalize3(a) ((a).r / 255.0), ((a).g / 255.0), ((a).b / 255.0)
#define vp_rgbnormalize(x) ((x) / 255.0)

typedef enum{
    vp_OK,
    vp_FAIL
} vp_result;

// Color definitions
typedef struct vp_rgb{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} vp_rgb_t;

// ### Throws an error.
// #### (const char* msg, int status)
// This should only be used by vapor functions.
void vp_throw(const char* msg, int status);

// ### Writes to a file with a specific mode, may be unsafe depending on the usage.
// #### (const char* File name, const char* Data to write, const char* mode)
// Writes to a file, returns vp_OK or vp_FAIL depending if successful
vp_result vp_Mwritefile(const char* fileName, const char* data, const char* mode);

// ### Rewrites a file.
// #### (const char* File name, const char* Data to write)
// Writes to a file, returns vp_OK or vp_FAIL depending if successful
vp_result vp_RWwritefile(const char* fileName, const char* data);

// ### Appends to a file.
// #### (const char* File name, const char* Data to write)
// Writes to a file, returns vp_OK or vp_FAIL depending if successful
vp_result vp_Awritefile(const char* fileName, const char* data);

/** @brief Reads from a file
  * @param char* Destination string pointer
  * @param const_char* File name
  * @param const_size_t Buffer size
  * Reads from a file into a const char*, sets the string to null if file not found.
  * @return Returns the size of the file.
  */
size_t vp_readfileS(char* ptr, const char* fileName, const size_t bufSize);

size_t vp_readfileM(char* ptr, const char* fileName, const size_t bufSize, const char* mode);

// ### Gets the size of a file.
// #### (const char* File name)
size_t vp_getFileSize(const char* fileName);

// ### Min (Double)
// #### (double Value, double Bounds)
double vp_mind(double x, double b);

// ### Min (Int)
// #### (int Value, int Bounds)
int vp_mini(int x, int b);

// ### Max (Double)
// #### (double Value, double Bounds)
double vp_maxd(double x, double b);

// ### Max (Int)
// #### (int Value, int Bounds)
int vp_maxi(int x, int b);

// ### Clamp (Double)
// #### (double Value, double Min, double Max)
// Clamps a value between min and max.
double vp_clampd(double x, double min, double max);

// ### Clamp (Int)
// #### (int Value, int Min, int Max)
// Clamps a value between min and max.
int vp_clampi(int x, int min, int max);

// ### Compare (Float)
// #### (float a , float b, double Dead Point)
// Safely compares a float using the Dead Point.
int vp_floatcmp(float a, float b, double deadPoint);

// ### Compare (Double)
// #### (double a , double b, double Dead Point)
// Safely compares a float using the Dead Point.
int vp_doublecmp(double a, double b, double deadPoint);

// ### Lerp (Double)
// #### (double Starting point, double End point, double Weight)
// Interpolates based on the weight factor.
// Weight is NOT clamped 0.0-1.0
double vp_lerpd(double a, double b, double weight);

// ### Lerp (Double, Clamped)
// #### (double Starting point, double End point, double Weight)
// Weight gets clamped to 0.0 - 1.0
double vp_lerpd_c(double a, double b, double weight);

// ### Calculate Pi
// #### (int Iterations)
// Calculates PI based on the Gauss-Legendre algorithm.
// (You can also use the vp_PI constant)
double vp_calculate_pi(int iterations);

#ifndef __VAPOR_H_EXC_INI

// #### Don't touch
//void vp_iniMakeBuffer(size_t size);


// ### Cleans up after ini
// If you run 'vp_iniPrepareBuffer()', it's guaranteed to clean up
// automatically after you, but it's more recommended to explicitly call it.
// Incase you hit the '32 function limit', you can define '__VP_ININOCLEANUP'
// But after that you have to cleanup yourself
void vp_iniClean();

// ### Uses a file for preparation
// #### (const char* File name)
// Second step, loads in the file you selected into a buffer.
void vp_iniUseFile(const char* fileName);

// ### Prepares the buffer.
// Last step, lexes and sorts the buffer into a usable format. 
void vp_iniPrepareBuffer();

float vp_iniGetNumber(const char* key, const char* section);
const char* vp_iniGetString(const char* key, const char* section);
int vp_iniGetBool(const char* key, const char* section);

#endif

#endif