#include <edu_ualberta_med_scannerconfig_dmscanlib_ScanLib.h>
#include "DmScanLib.h"
#include "DmScanLibInternal.h"
#include "BarcodeInfo.h"

#include <iostream>

using namespace std;

const char * getResultCodeMsg(int resultCode) {
	const char * message = NULL;

	switch (resultCode) {
	case SC_SUCCESS:
		message = NULL;
		break;
	case SC_FAIL:
		message = "operation failed";
		break;
	case SC_TWAIN_UNAVAIL:
		message = "twain driver unavailable";
		break;
	case SC_INVALID_DPI:
		message = "invalid DPI specified";
		break;
	case SC_INVALID_PLATE_NUM:
		message = "invalid plate number specified";
		break;
	case SC_INVALID_VALUE:
		message = "invalid value specified";
		break;
	case SC_INVALID_IMAGE:
		message = "invalid image scanned";
		break;
	case SC_INCORRECT_DPI_SCANNED:
		message = "incorrect DPI on scanned image";
		break;
	default:
		message = "undefined error";
		break;
	}
	return message;
}

jobject createScanResultObject(JNIEnv * env, int resultCode, int value) {
	jclass scanLibResultClass = env->FindClass(
			"edu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult");

	// run the following command to obtain method signatures from a class.
	// javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.ScanLibResult
	jmethodID cons = env->GetMethodID(scanLibResultClass, "<init>",
			"(IILjava/lang/String;)V");

	jvalue data[3];
	data[0].i = resultCode;
	data[1].i = value;
	data[2].l = env->NewStringUTF(getResultCodeMsg(resultCode));

	return env->NewObjectA(scanLibResultClass, cons, data);
}

jobject createDecodeResultObject(JNIEnv * env, int resultCode,
		vector<BarcodeInfo *> * barcodes) {
	jclass resultClass = env->FindClass(
			"edu/ualberta/med/scannerconfig/dmscanlib/DecodeResult");

	// run the following command to obtain method signatures from a class.
	// javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.DecodeResult
	jmethodID cons = env->GetMethodID(resultClass, "<init>",
			"(IILjava/lang/String;)V");

	jvalue data[3];
	data[0].i = resultCode;
	data[1].i = 0;
	data[2].l = env->NewStringUTF(getResultCodeMsg(resultCode));

	jobject resultObj = env->NewObjectA(resultClass, cons, data);

	jmethodID setCellMethod = env->GetMethodID(resultClass, "setCell",
			"(IILjava/lang/String;)V");

	if (barcodes != NULL) {
		for (unsigned i = 0, n = barcodes->size(); i < n; ++i) {
			BarcodeInfo & info = *(*barcodes)[i];
			jvalue data[3];

			data[0].i = info.getRow();
			data[1].i = info.getCol();
			data[2].l = env->NewStringUTF(info.getMsg().c_str());

			env->CallObjectMethodA(resultObj, setCellMethod, data);
		}
	}

	return resultObj;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slIsTwainAvailable
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_isTwainAvailable(
		JNIEnv * env, jobject obj) {
	DmScanLib dmScanLib;
	int result = dmScanLib.isTwainAvailable();
	return createScanResultObject(env, result, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slSelectSourceAsDefault
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_selectSourceAsDefault(
		JNIEnv * env, jobject obj) {
	DmScanLib dmScanLib;
	int result = dmScanLib.selectSourceAsDefault();
	return createScanResultObject(env, result, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slGetScannerCapability
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_getScannerCapability(
		JNIEnv * env, jobject obj) {
	DmScanLib dmScanLib;
	int result = dmScanLib.getScannerCapability();
	return createScanResultObject(env, SC_SUCCESS, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slScanImage
 * Signature: (JJIIDDDDLjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanImage(
		JNIEnv * env, jobject obj, jlong verbose, jlong dpi, jint brightness,
		jint contrast, jobject region, jstring _filename) {
	const char *filename = env->GetStringUTFChars(_filename, 0);
	env->ReleaseStringUTFChars(_filename, filename);
	return NULL;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slScanFlatbed
 * Signature: (JJIILjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanFlatbed(
		JNIEnv * env, jobject obj, jlong verbose, jlong dpi, jint brightness,
		jint contrast, jstring _filename) {
	const char *filename = env->GetStringUTFChars(_filename, 0);
	env->ReleaseStringUTFChars(_filename, filename);
	return NULL;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slDecodePlate
 * Signature: (JJIIJDDDDDJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_decodePlate(
		JNIEnv * env, jobject obj, jlong verbose, jlong dpi, jint brightness,
		jint contrast, jlong plateNum, jobject region, jdouble scanGap,
		jlong squareDev, jlong edgeThresh, jlong corrections,
		jdouble cellDistance, jdouble gapX, jdouble gapY, jlong profileA,
		jlong profileB, jlong profileC, jlong orientation) {
	return NULL;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slDecodeImage
 * Signature: (JJLjava/lang/String;DJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_decodeImage(
		JNIEnv * env, jobject obj, jlong verbose, jlong plateNum,
		jstring _filename, jdouble scanGap, jlong squareDev, jlong edgetThres,
		jlong corrections, jdouble cellDistance, jdouble gapX, jdouble gapY,
		jlong profileA, jlong profileB, jlong profileC, jlong orientation) {

	const char *filename = env->GetStringUTFChars(_filename, 0);
	DmScanLib dmScanLib;

	int result = dmScanLib.decodeImage(verbose, plateNum, filename, scanGap,
			squareDev, edgetThres, corrections, cellDistance, gapX, gapY,
			profileA, profileB, profileC, orientation);

	jobject resultObj;

	if (result == SC_SUCCESS) {
		vector<BarcodeInfo *> barcodes = dmScanLib.getBarcodes();
		resultObj = createDecodeResultObject(env, result, &barcodes);
	} else {
		resultObj = createDecodeResultObject(env, result, NULL);
	}
	env->ReleaseStringUTFChars(_filename, filename);
	return resultObj;
}
