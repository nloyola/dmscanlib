/*
 * Contains the code used in the library when building it for Linux.
 */

#include "DmScanLibJni.h"
#include "DmScanLibJniInternal.h"
#include "DmScanLib.h"

#include <iostream>

namespace dmscanlib {

namespace jni {

void getResultCodeMsg(int resultCode, std::string & message) {
    switch (resultCode) {
    case SC_SUCCESS:
        message = "";
        break;
    case SC_TWAIN_UNAVAIL:
        message = "Operation not supported on your operating system.";
        break;
    case SC_INVALID_IMAGE:
        message = "invalid image.";
        break;
    case SC_INVALID_DPI:
        case SC_INCORRECT_DPI_SCANNED:
        message = "invalid image DPI.";
        break;
    case SC_INVALID_NOTHING_DECODED:
        message = "No datamatrix barcodes detected in the image.";
        break;
    case SC_INVALID_NOTHING_TO_DECODE:
        message = "No wells to decode.";
        break;
    case SC_FAIL:
        default:
        message = "undefined error";
        break;
    }
}

} /* namespace */

} /* namespace */

JNIEXPORT jobject JNICALL Java_org_biobank_platedecoder_dmscanlib_ScanLib_selectSourceAsDefault(
        JNIEnv * env,
        jobject obj) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}

JNIEXPORT jobject JNICALL Java_org_biobank_platedecoder_dmscanlib_ScanLib_getScannerCapability(
        JNIEnv * env, jobject obj) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}

JNIEXPORT jobject JNICALL Java_org_biobank_platedecoder_dmscanlib_ScanLib_scanImage(
        JNIEnv * env,
        jobject obj,
        jlong _verbose,
        jlong _dpi,
        jint _brightness,
        jint _contrast,
        jdouble x,
        jdouble y,
        jdouble width,
        jdouble height,
        jstring _filename) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}

JNIEXPORT jobject JNICALL Java_org_biobank_platedecoder_dmscanlib_ScanLib_scanFlatbed(
        JNIEnv * env,
        jobject obj,
        jlong _verbose,
        jlong _dpi,
        jint _brightness,
        jint _contrast,
        jstring _filename) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}

JNIEXPORT jobject JNICALL Java_org_biobank_platedecoder_dmscanlib_ScanLib_scanAndDecode(
        JNIEnv * env,
        jobject obj,
        jlong _verbose,
        jlong _dpi,
        jint _brightness,
        jint _contrast,
        jdouble x,
        jdouble y,
        jdouble width,
        jdouble height,
        jobject _decodeOptions,
        jobjectArray _wellRects) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}
