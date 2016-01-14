/**
 * Implements the ImgScanner.
 *
 * This class performs all interfacing with the TWAIN driver to acquire images
 * from the scanner.
 *
 * Some portions of the implementation borrowed from EZTWAIN.
 *
 *--
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "ImgScannerTwain.h"

#include <math.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

namespace dmscanlib {

namespace imgscanner {

DSMENTRYPROC ImgScannerTwain::twainDsmEntry = NULL;

// Initialize g_AppID. This structure is passed to DSM_Entry() in each
// function call.
TW_IDENTITY ImgScannerTwain::twainAppID = {
   0,
   { 1, 0, TWLG_ENGLISH_USA, TWCY_USA, "dmscanlib 1.0" },
   TWON_PROTOCOLMAJOR, TWON_PROTOCOLMINOR, DG_CONTROL | DG_IMAGE,
   "Canadian Biosample Repository",
   "Image acquisition library", "dmscanlib",
};

ImgScannerTwain::ImgScannerTwain() {
   init();
}

ImgScannerTwain::~ImgScannerTwain() {
}

void ImgScannerTwain::init() {
   supportedDpis.init();
}

void ImgScannerTwain::setTwainDsmEntry(DSMENTRYPROC twainDsmEntry) {
   ImgScannerTwain::twainDsmEntry = twainDsmEntry;
   VLOG(1) << "ImgScannerTwain::setTwainDsmEntry";
}

/*
 * selectSourceAsDefault()
 * @params - none
 * @return - none
 *
 * Select the source to use as default for Twain, so the source does not
 * have to be specified every time.
 */
bool ImgScannerTwain::selectSourceAsDefault() {

   // Create a static window whose handle is passed to DSM_Entry() when we
   // open the data source manager.
   HWND hwnd = CreateWindowA("STATIC", "", WS_POPUPWINDOW, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                             HWND_DESKTOP, 0, 0 /* g_hinstDLL */, 0);

   CHECK_NOTNULL(hwnd); // Unable to create private window

   TW_UINT16 rc;
   TW_IDENTITY srcID;

   // Open the data source manager.
   rc = invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM,
                    (TW_MEMREF) &hwnd);

   if (rc != TWRC_SUCCESS) {
      return false;
   }

   // Display the "Select Source" dialog box for selecting a data source.
   ZeroMemory (&srcID, sizeof(srcID));
   rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, &srcID);

   CHECK_NE(rc, TWRC_FAILURE) << "Unable to display user interface ";

   // Close the data source manager.
   invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &hwnd);
   DestroyWindow(hwnd);

   if (rc == TWRC_CANCEL) {
      return false;
   }

   VLOG(3) << "selectSourceAsDefault: " << srcID.ProductName;
   init();
   return true;
}

void ImgScannerTwain::getValidDpis(std::vector<int> & validDpis) {
   CHECK(false) << "implement this method";
}

/* Assuming x-y resolution are the same*/
int ImgScannerTwain::getScannerCapability() {
   TW_IDENTITY srcID;
   HWND hwnd;

   if (!scannerSourceInit(hwnd, srcID)) {
      return 0;
   }

   int capabilityCode = getScannerCapabilityInternal(srcID);
   scannerSourceDeinit(hwnd, srcID);
   VLOG(5) << "Capability code: " << capabilityCode;

   return capabilityCode;
}

void ImgScannerTwain::getDeviceNames(std::vector<std::string> & names) {
   // do nothing - only used in Linux
}

void ImgScannerTwain::selectDevice(const std::string & name) {
   // do nothing - only used in Linux
}

void ImgScannerTwain::getFlatbedDimensionsInInches(std::pair<float, float> & dimensions) {
   HWND hwnd;
   TW_IDENTITY srcID;

   if (!scannerSourceInit(hwnd, srcID)) {
      errorCode = SC_FAIL;
      return;
   }

   dimensions.first = static_cast<float>(getCapabilityOneValue(srcID, ICAP_PHYSICALWIDTH));
   dimensions.second = static_cast<float>(getCapabilityOneValue(srcID, ICAP_PHYSICALHEIGHT));

   VLOG(5) << "Flatbed dimensions: " << dimensions.first << " by " << dimensions.second;

   scannerSourceDeinit(hwnd, srcID);
}

/*
 * @params - none
 * @return - Image acquired from twain source, in dmtxImage format
 *
 * Grab an image from the twain source and convert it to the dmtxImage format
 */
std::unique_ptr<Image> ImgScannerTwain::acquireImage(unsigned dpi,
                                                     int brightness,
                                                     int contrast,
                                                     float left,
                                                     float top,
                                                     float right,
                                                     float bottom) {
   TW_UINT16 rc;
   TW_UINT32 handle = 0;
   TW_IDENTITY srcID;
   HWND hwnd;
   Capability capability;

   CHECK_EQ(sizeof(TW_FIX32), sizeof(long));

   if (!scannerSourceInit(hwnd, srcID)) {
      errorCode = SC_FAIL;
      return std::unique_ptr<Image>();
   }

   VLOG(3) << "acquireImage: source: \"" << srcID.ProductName << "\""
           << ", brightness/" << brightness
           << ", constrast/" << contrast
           << ", left: " << left
           << ", top: " << top
           << ", right: " << right
           << ", bottom: " << bottom;

   int scannerCapability = getScannerCapabilityInternal(srcID);

   if (!(scannerCapability & CAP_IS_SCANNER)) {
      scannerSourceDeinit(hwnd, srcID);
      errorCode = SC_FAIL;
      return std::unique_ptr<Image>();
   }

   if (!supportedDpis.isValid(static_cast<double>(dpi))) {
      scannerSourceDeinit(hwnd, srcID);
      errorCode = SC_INVALID_DPI;
      return std::unique_ptr<Image>();
   }

   getCapabilityInternal(srcID, ICAP_BRIGHTNESS, capability);
   CHECK(capability.isValid(brightness)) << "brightness value is not valid: " << brightness;

   getCapabilityInternal(srcID, ICAP_CONTRAST, capability);
   CHECK(capability.isValid(contrast)) << "contrast value is not valid: " << contrast;

   double physicalWidth = getCapabilityOneValue(srcID, ICAP_PHYSICALWIDTH);
   double physicalHeight = getCapabilityOneValue(srcID, ICAP_PHYSICALHEIGHT);

   CHECK_GE(left, 0) << "bounding box exeeds flatbed dimensions";
   CHECK_GE(top, 0) << "bounding box exeeds flatbed dimensions";
   CHECK_LE(right, physicalWidth) << "bounding box exeeds flatbed dimensions";
   CHECK_LE(bottom, physicalHeight) << "bounding box exeeds flatbed dimensions";

   CHECK_GT(right, left) << "bounding box exeeds flatbed dimensions";
   CHECK_GT(bottom, top) << "bounding box exeeds flatbed dimensions";

   if ((scannerCapability & CAP_IS_WIA) == CAP_IS_WIA) {
      // WIA drivers use a rectangle to define the scanning region,
      //
      // i.e. the top left corner and a widht and height
      right -= left;
      bottom -= top;
   }

   errorCode = SC_SUCCESS;

   setCapOneValue(&srcID, ICAP_UNITS, TWTY_UINT16, TWUN_INCHES);
   setCapOneValue(&srcID, ICAP_COMPRESSION, TWTY_UINT16, TWCP_NONE);
   setCapOneValueFixedPoint(&srcID, ICAP_XRESOLUTION, dpi, 0);
   setCapOneValueFixedPoint(&srcID, ICAP_YRESOLUTION, dpi, 0);
   setCapOneValue(&srcID, ICAP_PIXELTYPE, TWTY_UINT16, TWPT_RGB);
   setCapOneValueFixedPoint(&srcID, ICAP_BRIGHTNESS, brightness, 0);
   setCapOneValueFixedPoint(&srcID, ICAP_CONTRAST, contrast, 0);

   TW_IMAGELAYOUT layout;
   setFloatToIntPair(left, layout.Frame.Left.Whole, layout.Frame.Left.Frac);
   setFloatToIntPair(top, layout.Frame.Top.Whole, layout.Frame.Top.Frac);
   setFloatToIntPair(right, layout.Frame.Right.Whole, layout.Frame.Right.Frac);
   setFloatToIntPair(bottom, layout.Frame.Bottom.Whole, layout.Frame.Bottom.Frac);
   layout.DocumentNumber = 1;
   layout.PageNumber = 1;
   layout.FrameNumber = 1;
   rc = invokeTwain(&srcID, DG_IMAGE, DAT_IMAGELAYOUT, MSG_SET, &layout);

   //Prepare to enable the default data source
   TW_USERINTERFACE ui;
   ui.ShowUI = FALSE;
   ui.ModalUI = FALSE;
   ui.hParent = hwnd;

   // Enable the default data source.
   rc = invokeTwain(&srcID, DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS, &ui);
   VLOG(3) << "DG_CONTROL / DAT_USERINTERFACE / MSG_ENABLEDS";
   CHECK_EQ(rc, TWRC_SUCCESS) << "Unable to enable default data source";

   if (rc == TWRC_FAILURE) {
      errorCode = SC_FAIL;
      scannerSourceDeinit(hwnd, srcID);
      VLOG(3) << "TWRC_FAILURE";
      return std::unique_ptr<Image>();
   }

   MSG msg;
   TW_EVENT event;
   TW_PENDINGXFERS pxfers;

   while (GetMessage((LPMSG) &msg, 0, 0, 0)) {
      event.pEvent = (TW_MEMREF) &msg;
      event.TWMessage = MSG_NULL;

      rc = invokeTwain(&srcID, DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, &event);
      if (rc == TWRC_NOTDSEVENT) {
         TranslateMessage((LPMSG) &msg);
         DispatchMessage((LPMSG) &msg);
         continue;
      }

      if (event.TWMessage == MSG_CLOSEDSREQ) {
         rc = invokeTwain(&srcID, DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS, &ui);
         VLOG(3) << "got MSG_CLOSEDSREQ: sending DG_CONTROL / DAT_USERINTERFACE / MSG_DISABLEDS";
         break;
      }

      if (event.TWMessage == MSG_XFERREADY) {
         TW_IMAGEINFO ii;

         rc = invokeTwain(&srcID, DG_IMAGE, DAT_IMAGEINFO, MSG_GET, &ii);
         VLOG(3) << "DG_IMAGE / DAT_IMAGEINFO / MSG_GET";

         if (rc == TWRC_FAILURE) {
            invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pxfers);
            VLOG(3) << "DG_CONTROL / DAT_PENDINGXFERS / MSG_RESET";
            LOG(WARNING) << "Unable to obtain image information";
            break;
         }

         // If image is compressed or is not 8-bit color and not 24-bit
         // color ...
         if ((rc != TWRC_CANCEL)
             && ((ii.Compression != TWCP_NONE)
                 || ((ii.BitsPerPixel != 8) && (ii.BitsPerPixel != 24)))) {
            invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pxfers);
            LOG(WARNING) << "Image compressed or not 8-bit/24-bit ";
            break;
         }

         //debug info
         VLOG(3) << "acquire:"
                 << " XResolution/" << ii.XResolution.Whole << "." << ii.XResolution.Frac
                 << " YResolution/" << ii.YResolution.Whole << "." << ii.YResolution.Frac
                 << " imageWidth/" << ii.ImageWidth
                 << " imageLength/" << ii.ImageLength
                 << " SamplesPerPixel/" << ii.SamplesPerPixel
                 << " bits per pixel/" << ii.BitsPerPixel
                 << " compression/" << ii.Compression
                 << " pixelType/" << ii.PixelType;

         // Perform the transfer.
         rc = invokeTwain(&srcID, DG_IMAGE, DAT_IMAGENATIVEXFER, MSG_GET, &handle);
         VLOG(3) << "DG_IMAGE / DAT_IMAGENATIVEXFER / MSG_GET";

         // If image not successfully transferred ...
         if (rc != TWRC_XFERDONE) {
            invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pxfers);
            VLOG(3) << "DG_CONTROL / DAT_PENDINGXFERS / MSG_RESET";
            LOG(WARNING) << "User aborted transfer or failure";
            errorCode = SC_INVALID_IMAGE;
            handle = 0;
            break;
         }

         // acknowledge end of transfer.
         rc = invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, &pxfers);
         VLOG(3) << "DG_CONTROL / DAT_PENDINGXFERS / MSG_ENDXFER";

         if (rc == TWRC_SUCCESS) {
            if (pxfers.Count != 0) {
               // Cancel all remaining transfers.
               invokeTwain(&srcID, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pxfers);
               VLOG(3) << "DG_CONTROL / DAT_PENDINGXFERS / MSG_RESET";
            } else {
               rc = invokeTwain(&srcID, DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS, &ui);
               VLOG(3) << "DG_CONTROL / DAT_USERINTERFACE / MSG_DISABLEDS";
               break;
            }
         }
      }
   }

   VLOG(2) << "image acquired successfully";

   scannerSourceDeinit(hwnd, srcID);
   if (handle != 0) {
	   return std::unique_ptr<Image>(new Image(reinterpret_cast<HANDLE>(handle)));
   }
   return std::unique_ptr<Image>();
}

std::unique_ptr<Image> ImgScannerTwain::acquireFlatbed(unsigned dpi,
                                                       int brightness,
                                                       int contrast) {
   TW_IDENTITY srcID;
   HWND hwnd;

   errorCode = SC_FAIL;

   if (!scannerSourceInit(hwnd, srcID)) {
      return 0;
   }

   float physicalWidth =
      static_cast<float>(getCapabilityOneValue(srcID, ICAP_PHYSICALWIDTH));
   float physicalHeight =
      static_cast<float>(getCapabilityOneValue(srcID, ICAP_PHYSICALHEIGHT));

   scannerSourceDeinit(hwnd, srcID);
   return acquireImage(dpi, brightness, contrast, 0, 0, physicalWidth, physicalHeight);
}

unsigned ImgScannerTwain::invokeTwain(TW_IDENTITY * srcId,
                                      unsigned long dg,
                                      unsigned dat,
                                      unsigned msg,
                                      void * ptr) {
   CHECK_NOTNULL(twainDsmEntry);
   unsigned r = twainDsmEntry(&twainAppID, srcId, dg, dat, msg, ptr);
   VLOG(5) << "invokeTwain: srcId: \""
           << ((srcId != NULL) ? srcId->ProductName : "NULL")
           << "\", dg: " << dg
           << ", dat: " << dat
           << ", msg: " << msg
           << ", ptr: " << ptr
           << ", returnCode: " << r;

   if ((srcId == NULL) && (r != TWRC_SUCCESS) && (r != TWRC_CHECKSTATUS)) {
      VLOG(3) << "ImgScannerTwain::invokeTwain: unsuccessful call to twain";
   }
   return r;
}

void ImgScannerTwain::setFloatToIntPair(double f,
                                        short & whole,
                                        unsigned short & frac) {
   double round = (f > 0) ? 0.5 : -0.5;
   const unsigned tmp = static_cast<unsigned> (f * 65536.0 + round);
   whole = static_cast<short> (tmp >> 16);
   frac = static_cast<unsigned short> (tmp & 0xffff);
}

BOOL ImgScannerTwain::setCapOneValue(TW_IDENTITY * srcId,
                                     unsigned Cap,
                                     unsigned ItemType,
                                     unsigned long ItemVal) {
   BOOL ret_value = FALSE;
   TW_CAPABILITY cap;
   pTW_ONEVALUE pv;

   cap.Cap = Cap; // capability id
   cap.ConType = TWON_ONEVALUE; // container type
   cap.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
   CHECK_NOTNULL(cap.hContainer);

   pv = (pTW_ONEVALUE) GlobalLock(cap.hContainer);
   CHECK_NOTNULL(pv);

   pv->ItemType = ItemType;
   pv->Item = ItemVal;
   GlobalUnlock(cap.hContainer);
   ret_value = invokeTwain(srcId, DG_CONTROL, DAT_CAPABILITY, MSG_SET, &cap);
   GlobalFree(cap.hContainer);
   return ret_value;
}

BOOL ImgScannerTwain::setCapOneValueFixedPoint(TW_IDENTITY * srcID,
                                               unsigned Cap,
                                               TW_INT16 whole,
                                               TW_UINT16 frac) {
   TW_FIX32 value;
   value.Whole = whole;
   value.Frac = frac;
   if (unsigned long * longval = reinterpret_cast<unsigned long *>(&value)) {
      return setCapOneValue(srcID, Cap, TWTY_FIX32, *longval);
   }
   CHECK(FALSE) << "reinterpret cast failed";
   return false;
}

double ImgScannerTwain::getCapabilityOneValue(TW_IDENTITY & srcID, TW_UINT16 cap) {
   VLOG(2) << "getCapabilityOneValue: srcId:" << srcID.ProductName
           << ", cap: " << cap;

   Capability capability;
   getCapabilityInternal(srcID, cap, capability);
   CHECK_EQ(capability.values.size(), 1) << "one value capability could not be retrieved";
   return capability.values[0];
}

inline double ImgScannerTwain::uint32ToFloat(TW_UINT32 uint32) {
   TW_FIX32 fix32 = *((pTW_FIX32) (void *) (&uint32));
   return twfix32ToFloat(fix32);
}

inline double ImgScannerTwain::twfix32ToFloat(TW_FIX32 fix32) {
   return static_cast<float> (fix32.Whole) + static_cast<float> (fix32.Frac) / 65536.0;
}

void ImgScannerTwain::getCustomDsData(TW_IDENTITY * srcId) {
   TW_UINT16 rc;
   TW_CUSTOMDSDATA cdata;

   rc = invokeTwain(srcId, DG_CONTROL, DAT_CUSTOMDSDATA, MSG_GET, &cdata);
   if (rc == TWRC_SUCCESS) {
      //char * o = (char *)GlobalLock(cdata.hData);
      GlobalUnlock(cdata.hData);
      GlobalFree(cdata.hData);
   }
}

/*
 * Opens the default data source.
 */
bool ImgScannerTwain::scannerSourceInit(HWND & hwnd, TW_IDENTITY & srcID) {
   TW_UINT16 rc;

   hwnd = CreateWindowA("STATIC", "", WS_POPUPWINDOW, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP,
			0, 0 /* g_hinstDLL */, 0);

   ShowWindow(hwnd, SW_HIDE);

   CHECK(hwnd != 0) << "Unable to create private window";

   SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);

   // Open the data source manager.
   rc = invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, &hwnd);
   CHECK_EQ(rc, TWRC_SUCCESS) << "Unable to open data source manager";

   // get the default source
   rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, &srcID);
   if (rc != TWRC_SUCCESS) {
      if (!selectSourceAsDefault()) {
         return false;
      }
   }

   rc = invokeTwain(NULL, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &srcID);
   if (rc != TWRC_SUCCESS) {
      // Unable to open default data source
      invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &hwnd);
      VLOG(3) << "DG_CONTROL / DAT_PARENT / MSG_CLOSEDSM";
      return false;
   }
   return true;
}

void ImgScannerTwain::scannerSourceDeinit(HWND & hwnd, TW_IDENTITY & srcID) {
   // Close the data source.
   invokeTwain(&srcID, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, &srcID);
   VLOG(3) << "DG_CONTROL / DAT_IDENTITY / MSG_CLOSEDS";

   // Close the data source manager.
   invokeTwain(NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &hwnd);
   VLOG(3) << "DG_CONTROL / DAT_PARENT / MSG_CLOSEDSM";

   // Destroy window.
   DestroyWindow(hwnd);
}

int ImgScannerTwain::getScannerCapabilityInternal(TW_IDENTITY & srcID) {
   Capability capability;
   int capabilityCode = 0, physicalwidth = 0, physicalheight = 0;

   setCapOneValue(&srcID, ICAP_UNITS, TWTY_UINT16, TWUN_INCHES);
   getCapabilityInternal(srcID, ICAP_XRESOLUTION, supportedDpis);

   VLOG(5) << "Polling driver for driver type";

   if (srcID.ProductName != NULL) {
      int len = strlen(srcID.ProductName);
      char prodNameLower[1024];

	  CHECK_LT(len, 1024) << "product name length exceeds hardcoded size";

      for (int i = 0; i < len; ++i) {
         prodNameLower[i] = tolower(srcID.ProductName[i]);
      }
      prodNameLower[len] = 0;

      if (strstr(prodNameLower, "wia") != NULL) {
         VLOG(5) << "Driver type is WIA: ProductName: " << prodNameLower;
         capabilityCode |= CAP_IS_WIA;
      } else {
         VLOG(5) << "Driver type is TWAIN (default): ProductName: " << prodNameLower;
      }
   }

   if (!supportedDpis.values.empty() || (supportedDpis.range.min > 0)) {
      capabilityCode |= CAP_IS_SCANNER;
   }
   return capabilityCode;
}

void ImgScannerTwain::getCapabilityInternal(TW_IDENTITY & srcID,
                                            TW_UINT16 cap,
                                            Capability & capability) {
   VLOG(5) << "Polling scanner capbility";

   pTW_RANGE pvalRange;
   TW_UINT16 rc;
   int capabilityCode = 0;
   TW_CAPABILITY twCap;

   twCap.Cap        = cap;
   twCap.ConType    = TWON_DONTCARE16;
   twCap.hContainer = NULL;

   rc = invokeTwain(&srcID, DG_CONTROL, DAT_CAPABILITY, MSG_GET, &twCap);
   CHECK(rc == TWRC_SUCCESS) << "Failed to obtain capability values";

   VLOG(5) << "twCap.ConType = " << twCap.ConType;

   switch (twCap.ConType) {

      case TWON_RANGE: {

         VLOG(5) << "ConType = Range";

         pvalRange = (pTW_RANGE) GlobalLock(twCap.hContainer);
         if (pvalRange->ItemType == TWTY_FIX32) {
            capability.set(uint32ToFloat(pvalRange->MinValue),
                           uint32ToFloat(pvalRange->MaxValue),
                           uint32ToFloat(pvalRange->StepSize));

            VLOG(7) << "capability range ["
                    << " min:"  << capability.range.min
                    << " max:"  << capability.range.max
                    << " step:" << capability.range.step
                    << " ]";
         }
         break;
      }

      case TWON_ENUMERATION: {
         VLOG(5) << "ConType = Enumeration";

         pTW_ENUMERATION pvalEnum;
         TW_UINT16 index;
         double value = 0;

         pvalEnum = (pTW_ENUMERATION) GlobalLock(twCap.hContainer);
         CHECK_NOTNULL(pvalEnum);

         VLOG(5) << "number of items in enumaration: " << pvalEnum->NumItems;
         VLOG(5) << "item type: " << pvalEnum->ItemType;

         for (index = 0; index < pvalEnum->NumItems; index++) {
            CHECK_EQ(pvalEnum->ItemType, TWTY_FIX32)
               << "invalid item type: " << pvalEnum->ItemType;

            value =
               twfix32ToFloat(*(TW_FIX32 *) (void *) (&pvalEnum->ItemList[index * 4]));
            VLOG(5) << "value " << index << " (f32bit): " << value;

            capability.addValue(value);
         }
         break;
      }

         //XXX Untested
      case TWON_ONEVALUE: {
         VLOG(5) << "ConType = OneValue";

         pTW_ONEVALUE pvalOneValue;

         pvalOneValue = (pTW_ONEVALUE) GlobalLock(twCap.hContainer);

         CHECK_EQ(pvalOneValue->ItemType, TWTY_FIX32) <<
            "invalid item type: " << pvalOneValue->ItemType;

         double value =
			 twfix32ToFloat(*(TW_FIX32 *) (void *) (&pvalOneValue->Item));

         VLOG(6) << "single value (f32bit): " << value;

         capability.addValue(value);
         break;
      }

      default:
         LOG(WARNING) << "Unexpected dpi contype";
         break;
   }
   GlobalUnlock(twCap.hContainer);
   GlobalFree(twCap.hContainer);
}

} /* namespace */

} /* namespace */
