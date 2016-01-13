#include "imgscanner/SaneUtil.h"

#include <glog/logging.h>

SANE_Handle SaneUtil::openDevice(std::string & deviceName) {
   CHECK_GE(deviceName.length(), 0) << "no device selected";

   SANE_Status status = sane_init(NULL, NULL);
   CHECK_EQ(status, SANE_STATUS_GOOD) << "SANE initialization failed: " << status;

   SANE_Handle handle;
   status = sane_open(deviceName.c_str(), &handle);
   CHECK_EQ(status, SANE_STATUS_GOOD) << "SANE open failed: " << status;

   return handle;
}

void SaneUtil::closeDevice(SANE_Handle handle) {
   sane_close(handle);
   sane_exit();
}

SANE_Word SaneUtil::getControlOptionWord(SANE_Handle saneHandle,
                                         SANE_Int optNum) {
   SANE_Word value;
   SANE_Status status = sane_control_option(saneHandle,
                                            optNum,
                                            SANE_ACTION_GET_VALUE,
                                            &value,
                                            NULL);
   CHECK_EQ(status, SANE_STATUS_GOOD)
      << "could not get value for option: " << optNum
      << ", status: " << status;
   return value;
}

void SaneUtil::setControlOptionWord(SANE_Handle saneHandle,
                                    SANE_Int    optNum,
                                    SANE_Word   value) {
   VLOG(5) << "setControlOptionWord: handle: " << saneHandle
           << ", option: " << optNum
           << ", value: " << value;
   SANE_Status status = sane_control_option(saneHandle,
                                            optNum,
                                            SANE_ACTION_SET_VALUE,
                                            &value,
                                            NULL);
   CHECK_EQ(status, SANE_STATUS_GOOD)
      << "could not set value for option: " << optNum
      << ", status: " << status;
}

SANE_Word* SaneUtil::getControlOptionWordArray(SANE_Handle saneHandle,
                                               SANE_Int    optNum,
                                               SANE_Word*  arr) {
   SANE_Status status = sane_control_option(saneHandle,
                                            optNum,
                                            SANE_ACTION_GET_VALUE,
                                            arr,
                                            NULL);
   CHECK_EQ(status, SANE_STATUS_GOOD)
      << "could not get value for option: " << optNum
      << ", status: " << status;
   return arr;
}

SANE_String SaneUtil::getControlOptionString(SANE_Handle saneHandle,
                                             SANE_Int    optNum,
                                             SANE_String str) {
   SANE_Status status = sane_control_option(saneHandle,
                                            optNum,
                                            SANE_ACTION_GET_VALUE,
                                            str,
                                            NULL);
   CHECK_EQ(status, SANE_STATUS_GOOD)
      << "could not get value for option: " << optNum
      << ", status: " << status;
   return str;
}

void SaneUtil::setControlOptionString(SANE_Handle saneHandle,
                                      SANE_Int    optNum,
                                      SANE_String str) {
   SANE_Status status = sane_control_option(saneHandle,
                                            optNum,
                                            SANE_ACTION_SET_VALUE,
                                            str,
                                            NULL);
   VLOG(5) << "setControlOptionString: handle: " << saneHandle
           << ", option: " << optNum
           << ", value: " << str;

   CHECK_EQ(status, SANE_STATUS_GOOD)
      << "could not set value for option: " << optNum
      << ", status: " << status;
}

int SaneUtil::scaleValue(int value, int slopeRise, int slopeRun) {
   double slope = static_cast<double>(slopeRise) / static_cast<double>(slopeRun);
   return static_cast<int>(slope * value);
}
