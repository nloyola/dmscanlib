/*
 * Implementation for JNI functions.
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "jni/DmScanLibJni.h"
#include "DmScanLib.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellDecoder.h"

#include <map>
#include <memory>
#include <glog/logging.h>

namespace dmscanlib {

namespace jni {

const char * const CLASS_SCAN_LIB_RESULT     = "org/biobank/dmscanlib/ScanLibResult";
const char * const CLASS_VALUE_RESULT        = "org/biobank/dmscanlib/ValueResult";
const char * const CLASS_VALID_DPIS_RESULT   =  "org/biobank/dmscanlib/ValidDpisResult";
const char * const CLASS_DECODE_RESULT       = "org/biobank/dmscanlib/DecodeResult";
const char * const CLASS_DEVICE_NAMES_RESULT = "org/biobank/dmscanlib/DeviceNamesResult";

const char * const getResultCodeMsg(int resultCode) {
   switch (resultCode) {
      case SC_SUCCESS:
         return "";
      case SC_FAIL:
         return "operation failed";
      case SC_TWAIN_UNAVAIL:
         return "twain driver unavailable";
      case SC_INVALID_DPI:
         return "invalid DPI specified";
      case SC_INVALID_NOTHING_DECODED:
         return "no datamatrix barcodes could be decoded from the image";
      case SC_INVALID_IMAGE:
         return "invalid image scanned";
      case SC_INVALID_NOTHING_TO_DECODE:
         return "no wells to decode";
      case SC_INCORRECT_DPI_SCANNED:
         return "incorrect DPI on scanned image";
      case SC_INVALID_DEVICE:
         return "invalid device selected";
      case SC_INVALID_BRIGHTNESS:
         return "invalid brightness";
      case SC_INVALID_CONTRAST:
         return "invalid contrast";
      default:
         break;
   }

   return "undefined error";
}

/**
 * Creates a class derived from org.biobank.dmscanlib.ScanLibResult.
 */
jobject createScanLibResultDerivedObject(JNIEnv * env,
                                         int resultCode,
                                         jclass & resultClass) {
   // run the following command to obtain method signatures from a class.
   // javap -s -p org.biobank.dmscanlib.DecodeResult
   jmethodID cons = env->GetMethodID(resultClass, "<init>", "(ILjava/lang/String;)V");
   CHECK(cons != NULL) << "could not find JNI constructor";

   jvalue data[2];
   data[0].i = resultCode;
   data[1].l = env->NewStringUTF(getResultCodeMsg(resultCode));

   jobject jobj = env->NewObjectA(resultClass, cons, data);
   CHECK(jobj != NULL) << "could not create JNI object";
   return jobj;
}

jobject createScanResultObject(JNIEnv * env, int resultCode) {
   jclass resultClass = env->FindClass(CLASS_SCAN_LIB_RESULT);
   CHECK(resultClass != NULL) << "could not find JNI class";

   return createScanLibResultDerivedObject(env, resultCode, resultClass);
}

jobject createValueResultObject(JNIEnv * env, int resultCode, int value) {
   jclass resultClass = env->FindClass(CLASS_VALUE_RESULT);
   CHECK(resultClass != NULL) << "could not find JNI class";

   // run the following command to obtain method signatures from a class.
   // javap -s -p org.biobank.dmscanlib.DecodeResult
   jmethodID cons = env->GetMethodID(resultClass, "<init>", "(ILjava/lang/String;I)V");
   CHECK(cons != NULL) << "could not find JNI constructor";

   jvalue data[3];
   data[0].i = resultCode;
   data[1].l = env->NewStringUTF(getResultCodeMsg(resultCode));
   data[2].i = value;

   jobject jobj = env->NewObjectA(resultClass, cons, data);
   CHECK(jobj != NULL) << "could not create JNI object";
   return jobj;
}

jobject createValidDpisResultObject(JNIEnv * env,
                                    int resultCode,
                                    const std::vector<int> validDpis) {
   jclass resultClass = env->FindClass(CLASS_VALID_DPIS_RESULT);
   CHECK(resultClass != NULL) << "could not find JNI class";

   jobject resultObj = createScanLibResultDerivedObject(env, resultCode, resultClass);

   if (!validDpis.empty()) {
      jmethodID method = env->GetMethodID(resultClass, "addDpi", "(I)V");
      CHECK(method != NULL) << "could not find JNI method";

      for (const int & dpi : validDpis) {
         jvalue data[1];
         data[0].i = dpi;
         env->CallObjectMethodA(resultObj, method, data);
      }
   }

   return resultObj;
}

jobject createDecodeResultObject(
   JNIEnv * env,
   int resultCode,
   const std::map<std::string, const dmscanlib::WellDecoder *> & wellDecoders) {

   jclass resultClass = env->FindClass(CLASS_DECODE_RESULT);
   CHECK(resultClass != NULL) << "could not find JNI class";

   jobject resultObj = createScanLibResultDerivedObject(env, resultCode, resultClass);

   if (!wellDecoders.empty()) {
      jmethodID method = env->GetMethodID(resultClass,
                                         "addWell",
                                         "(Ljava/lang/String;Ljava/lang/String;)V");

      CHECK(method != NULL) << "could not find JNI method";

      for (auto & kv : wellDecoders) {
         const dmscanlib::WellDecoder & wellDecoder = *kv.second;
         VLOG(5) << wellDecoder;

         jvalue data[3];
         data[0].l = env->NewStringUTF(wellDecoder.getLabel().c_str());
         data[1].l = env->NewStringUTF(wellDecoder.getMessage().c_str());

         env->CallObjectMethodA(resultObj, method, data);
      }

      VLOG(2) << "wells decoded: " << wellDecoders.size();
   }

   return resultObj;
}

jobject createDecodeResultObject(JNIEnv * env, int resultCode) {
   const std::map<std::string, const dmscanlib::WellDecoder *> wellDecoders;
   return createDecodeResultObject(env, resultCode, wellDecoders);
}


jobject createDeviceNamesResultObject(JNIEnv * env,
                                      int resultCode,
                                      const std::vector<std::string> & deviceNames) {
   jclass resultClass = env->FindClass(CLASS_DEVICE_NAMES_RESULT);
   CHECK(resultClass != NULL) << "could not find JNI class";

   jobject resultObj = createScanLibResultDerivedObject(env, resultCode, resultClass);

   if (!deviceNames.empty()) {
      jmethodID method = env->GetMethodID(resultClass,
                                         "addDeviceName",
                                         "(Ljava/lang/String;)V");
      CHECK(method != NULL) << "could not find JNI method";
      for (const std::string & name : deviceNames) {
         jvalue data[1];
         data[0].l = env->NewStringUTF(name.c_str());
         env->CallObjectMethodA(resultObj, method, data);
      }
   }

   return resultObj;
}

int getWellRectangles(JNIEnv *env,
                      jsize numWells,
                      jobjectArray _wellRects,
                      std::vector<std::unique_ptr<const WellRectangle> > & wellRects) {
   jobject wellRectJavaObj;
   jclass wellRectJavaClass = NULL;
   jmethodID wellRectGetLabelMethodID = NULL;
   jmethodID wellRectGetCornerXMethodID = NULL;
   jmethodID wellRectGetCornerYMethodID = NULL;

   VLOG(5) << "decodeImage: numWells/" << numWells;

   // TODO check for max well rectangle objects
   for (int i = 0; i < static_cast<int>(numWells); ++i) {
      wellRectJavaObj = env->GetObjectArrayElement(_wellRects, i);

      // if java object pointer is null, skip this array element
      if (wellRectJavaObj == NULL) {
         return 2;
      }

      if (wellRectJavaClass == NULL) {
         wellRectJavaClass = env->GetObjectClass(wellRectJavaObj);

         wellRectGetLabelMethodID = env->GetMethodID(wellRectJavaClass,
                                                    "getLabel", "()Ljava/lang/String;");
         if (env->ExceptionOccurred()) {
            VLOG(5) << "decodeImage: get method getLabel() failed";
            return 0;
         }

         wellRectGetCornerXMethodID = env->GetMethodID(wellRectJavaClass,
                                                      "getCornerX", "(I)D");
         if (env->ExceptionOccurred()) {
            return 0;
         }

         wellRectGetCornerYMethodID = env->GetMethodID(wellRectJavaClass,
                                                      "getCornerY", "(I)D");
         if (env->ExceptionOccurred()) {
            return 0;
         }
      }

      jobject labelJobj = env->CallObjectMethod(wellRectJavaObj, wellRectGetLabelMethodID);
      const char * label = env->GetStringUTFChars((jstring) labelJobj, NULL);

      double x1 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerXMethodID, 0);
      double y1 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerYMethodID, 0);

      double x2 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerXMethodID, 1);
      double y2 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerYMethodID, 1);

      double x3 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerXMethodID, 2);
      double y3 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerYMethodID, 2);

      double x4 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerXMethodID, 3);
      double y4 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerYMethodID, 3);

      double xmin = std::min(x1, std::min(x2, std::min(x3, x4)));
      double ymin = std::min(y1, std::min(y2, std::min(y3, y4)));

      double xmax = std::max(x1, std::max(x2, std::max(x3, x4)));
      double ymax = std::max(y1, std::max(y2, std::max(y3, y4)));

      std::unique_ptr<const WellRectangle> wellRect(
         new WellRectangle(label,
                           static_cast<unsigned>(xmin),
                           static_cast<unsigned>(ymin),
                           static_cast<unsigned>(xmax - xmin),
                           static_cast<unsigned>(ymax - ymin)));

      VLOG(5) << *wellRect;

      wellRects.push_back(std::move(wellRect));

      env->ReleaseStringUTFChars((jstring) labelJobj, label);
      env->DeleteLocalRef(wellRectJavaObj);
   }


   VLOG(5) << "decodeImage: success";
   return 1;
}

} /* namespace */

} /* namespace */

dmscanlib::DmScanLib dmScanLib;

JNIEXPORT jobject JNICALL
Java_org_biobank_dmscanlib_ScanLib_selectSourceAsDefault(JNIEnv * env, jobject thisObj) {
   int result = dmScanLib.selectSourceAsDefault();
   return dmscanlib::jni::createScanResultObject(env, result);
}

JNIEXPORT jobject JNICALL Java_org_biobank_dmscanlib_ScanLib_getDeviceNames(
   JNIEnv * env, jobject thisObj) {
   std::vector<std::string> names;
   dmScanLib.getDeviceNames(names);
   return dmscanlib::jni::createDeviceNamesResultObject(env, dmscanlib::SC_SUCCESS, names);
}

JNIEXPORT jobject JNICALL
Java_org_biobank_dmscanlib_ScanLib_getValidDpis(JNIEnv * env,
                                                jobject thisObj,
                                                jstring _deviceName) {

   const char * deviceName = env->GetStringUTFChars(_deviceName, 0);
   std::vector<int> validDpis;

   dmScanLib.selectDevice(deviceName);
   dmScanLib.getValidDpis(validDpis);

   env->ReleaseStringUTFChars(_deviceName, deviceName);
   return dmscanlib::jni::createValidDpisResultObject(env, dmscanlib::SC_SUCCESS, validDpis);
}

JNIEXPORT jobject JNICALL
Java_org_biobank_dmscanlib_ScanLib_getScannerCapability(JNIEnv * env, jobject thisObj) {
   int result = dmScanLib.getScannerCapability();
   return dmscanlib::jni::createValueResultObject(env, dmscanlib::SC_SUCCESS, result);
}

/**
 *
 *
 * @param left The left coordinate in inches.
 *
 * @param top The top coordinate in inches.
 *
 * @param right The right coordinate in inches.
 *
 * @param bottom The bottom coordinate in inches.
 */
JNIEXPORT jobject JNICALL
Java_org_biobank_dmscanlib_ScanLib_scanImage(JNIEnv * env,
                                             jobject thisObj,
                                             jlong _verbose,
                                             jstring _deviceName,
                                             jlong _dpi,
                                             jint _brightness,
                                             jint _contrast,
                                             jdouble left,
                                             jdouble top,
                                             jdouble right,
                                             jdouble bottom,
                                             jstring _filename) {
   if ((_dpi == 0) || (_deviceName == 0) || (_filename == 0)) {
      return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_FAIL);
   }

   unsigned verbose        = static_cast<unsigned>(_verbose);
   const char * deviceName = env->GetStringUTFChars(_deviceName, 0);
   unsigned dpi            = static_cast<unsigned>(_dpi);
   unsigned brightness     = static_cast<unsigned>(_brightness);
   unsigned contrast       = static_cast<unsigned>(_contrast);
   const char *filename    = env->GetStringUTFChars(_filename, 0);

   dmscanlib::DmScanLib::configLogging(verbose, false);
   int result = dmScanLib.scanImage(deviceName,
                                    dpi,
                                    brightness,
                                    contrast,
                                    static_cast<float>(left),
                                    static_cast<float>(top),
                                    static_cast<float>(right),
                                    static_cast<float>(bottom),
                                    filename);
   jobject resultObj = dmscanlib::jni::createScanResultObject(env, result);

   env->ReleaseStringUTFChars(_deviceName, deviceName);
   env->ReleaseStringUTFChars(_filename, filename);
   return resultObj;
}

JNIEXPORT jobject JNICALL
Java_org_biobank_dmscanlib_ScanLib_scanFlatbed(JNIEnv * env,
                                               jobject thisObj,
                                               jlong _verbose,
                                               jstring _deviceName,
                                               jlong _dpi,
                                               jint _brightness,
                                               jint _contrast,
                                               jstring _filename) {
   if ((_dpi == 0) || (_deviceName == 0) || (_filename == 0)) {
      return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_FAIL);
   }

   unsigned verbose        = static_cast<unsigned>(_verbose);
   const char * deviceName = env->GetStringUTFChars(_deviceName, 0);
   unsigned dpi            = static_cast<unsigned>(_dpi);
   unsigned brightness     = static_cast<unsigned>(_brightness);
   unsigned contrast       = static_cast<unsigned>(_contrast);
   const char *filename    = env->GetStringUTFChars(_filename, 0);

   dmscanlib::DmScanLib::configLogging(verbose, false);
   int result = dmScanLib.scanFlatbed(deviceName, dpi, brightness, contrast, filename);
   jobject resultObj = dmscanlib::jni::createScanResultObject(env, result);

   env->ReleaseStringUTFChars(_deviceName, deviceName);
   env->ReleaseStringUTFChars(_filename, filename);
   return resultObj;
}

JNIEXPORT jobject JNICALL
Java_org_biobank_dmscanlib_ScanLib_scanAndDecode(JNIEnv * env,
                                                 jobject thisObj,
                                                 jlong _verbose,
                                                 jstring _deviceName,
                                                 jlong _dpi,
                                                 jint _brightness,
                                                 jint _contrast,
                                                 jdouble left,
                                                 jdouble top,
                                                 jdouble right,
                                                 jdouble bottom,
                                                 jobject _decodeOptions,
                                                 jobjectArray _wellRects) {
   if ((_dpi == 0) || (_deviceName == 0) || (_decodeOptions == 0) || (_wellRects == 0)) {
      return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_FAIL);
   }

   unsigned verbose        = static_cast<unsigned>(_verbose);
   unsigned dpi            = static_cast<unsigned>(_dpi);
   const char * deviceName = env->GetStringUTFChars(_deviceName, 0);
   unsigned brightness     = static_cast<unsigned>(_brightness);
   unsigned contrast       = static_cast<unsigned>(_contrast);

   dmscanlib::DmScanLib::configLogging(verbose, false);

   std::vector<std::unique_ptr<const dmscanlib::WellRectangle> > wellRects;

   std::unique_ptr<dmscanlib::DecodeOptions> decodeOptions =
      dmscanlib::DecodeOptions::getDecodeOptionsViaJni(env, _decodeOptions);

   jsize numWells = env->GetArrayLength(_wellRects);
   int result = dmscanlib::jni::getWellRectangles(env, numWells, _wellRects, wellRects);

   if (result == 0) {
      // got an exception when converting from JNI
      return NULL;
   } else if ((result != 1) || (wellRects.size() == 0)) {
      // invalid rects or zero rects passed from java
      return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_INVALID_NOTHING_TO_DECODE);
   }

   result = dmScanLib.scanAndDecode(deviceName,
                                    dpi,
                                    brightness,
                                    contrast,
                                    static_cast<float>(left),
                                    static_cast<float>(top),
                                    static_cast<float>(right),
                                    static_cast<float>(bottom),
                                    *decodeOptions,
                                    wellRects);

   env->ReleaseStringUTFChars(_deviceName, deviceName);

   if (result == dmscanlib::SC_SUCCESS) {
      return dmscanlib::jni::createDecodeResultObject(env,result, dmScanLib.getDecodedWells());
   }
   return dmscanlib::jni::createDecodeResultObject(env, result);
}

JNIEXPORT jobject JNICALL
Java_org_biobank_dmscanlib_ScanLib_decodeImage(JNIEnv * env,
                                               jobject thisObj,
                                               jlong _verbose,
                                               jstring _filename,
                                               jobject _decodeOptions,
                                               jobjectArray _wellRects) {

   if ((_filename == 0) || (_decodeOptions == 0) || (_wellRects == 0)) {
      return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_FAIL);
   }

   unsigned verbose = static_cast<unsigned>(_verbose);
   const char *filename = env->GetStringUTFChars(_filename, 0);

   dmscanlib::DmScanLib::configLogging(verbose, false);

   std::unique_ptr<dmscanlib::DecodeOptions> decodeOptions =
      dmscanlib::DecodeOptions::getDecodeOptionsViaJni(env, _decodeOptions);
   std::vector<std::unique_ptr<const dmscanlib::WellRectangle> > wellRects;

   jsize numWells = env->GetArrayLength(_wellRects);
   int result = dmscanlib::jni::getWellRectangles(env, numWells, _wellRects, wellRects);

   if (result == 0) {
      // got an exception when converting from JNI
      VLOG(5) << "got an exception when converting from JNI";
      return NULL;
   } else if ((result != 1) || (wellRects.size() == 0)) {
      // invalid rects or zero rects passed from java
      return dmscanlib::jni::createDecodeResultObject(env,
                                                      dmscanlib::SC_INVALID_NOTHING_TO_DECODE);
   }

   result = dmScanLib.decodeImageWells(filename, *decodeOptions, wellRects);
   env->ReleaseStringUTFChars(_filename, filename);

   if (result == dmscanlib::SC_SUCCESS) {
      return dmscanlib::jni::createDecodeResultObject(env, result, dmScanLib.getDecodedWells());
   }
   return dmscanlib::jni::createDecodeResultObject(env, result);
}
