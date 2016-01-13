#ifndef __INCLUDE_SANE_UTIL_H
#define __INCLUDE_SANE_UTIL_H

#include <sane/sane.h>
#include <string>

class SaneUtil {
public:
   static SANE_Handle openDevice(std::string & deviceName);

   static void closeDevice(SANE_Handle handle);

   static SANE_Word getControlOptionWord(SANE_Handle saneHandle,
                                         SANE_Int    optNum);

   static void setControlOptionWord(SANE_Handle saneHandle,
                                    SANE_Int    optNum,
                                    SANE_Word   value);

   static SANE_Word* getControlOptionWordArray(SANE_Handle saneHandle,
                                               SANE_Int    optNum,
                                               SANE_Word*  arr);

   static SANE_String getControlOptionString(SANE_Handle saneHandle,
                                             SANE_Int    optNum,
                                             SANE_String str);

   static void setControlOptionString(SANE_Handle saneHandle,
                                      SANE_Int    optNum,
                                      SANE_String str);

   /**
    * Converts a value from a range using linear correlation.
    */
   static int scaleValue(int value, int slopeRise, int slopeRun);

};

/* Local Variables: */
/* mode: c++        */
/* End:             */

#endif /* __INCLUDE_SANE_UTIL_H */
