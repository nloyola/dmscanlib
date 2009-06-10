#ifndef __INC_SCANLIB_H
#define __INC_SCANLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#define EXPORT __declspec(dllexport)

typedef struct sScPixelLoc {
	int x;
	int y;
} ScPixelLoc;

typedef struct sScPixelFrame {
	int x0; // top
	int y0; // left
	int x1; // bottom
	int y1; // right
} ScPixelFrame;

/**
 * Specifies the region to scan.
 */
typedef struct sScFrame {
	int frameId;
	double x0; // top
	double y0; // left
	double x1; // bottom
	double y1; // right

} ScFrame;

const unsigned short SC_SUCCESS      = 0;
const unsigned short SC_FAIL         = 1;
const unsigned short SC_TWAIN_UAVAIL = 2;

EXPORT unsigned short slIsTwainAvailable();

EXPORT unsigned short scanImage(char * filename, double x0, double y0,
		double x1, double y1);

typedef unsigned short (FAR PASCAL *SL_ISTWAINAVAILABLE) ();

typedef unsigned short (FAR PASCAL *SL_SCANIMAGE) (char * filename, double x0,
		double y0,	double x1, double y1);

#ifdef __cplusplus
}
#endif

#endif /* __INC_SCANLIB_H */