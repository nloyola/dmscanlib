/*
 * Image.cpp
 *
 *  Created on: 2014-01-20
 *      Author: loyola
 */

#include "Image.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/types_c.h>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

namespace dmscanlib {

Image::Image(const std::string & _filename) : filename(_filename)
{
   image = cv::imread(filename.c_str());

   valid = (image.data != NULL);

   if (valid) {
      VLOG(3) << "Image::Image: width: " << image.cols
              << ", height: " << image.rows
              << ", depth: " << image.elemSize()
              << ", step: " << image.step1();
   }
}

Image::Image(const Image & that) : filename("") {
   if (that.image.data == NULL) {
      throw std::invalid_argument("parameter is null");
   }

   image = that.image;
   valid = true;

   VLOG(5) << "Image::Image: width: " << image.cols
           << ", height: " << image.rows
           << ", depth: " << image.elemSize()
           << ", step: " << image.step1();
}

Image::Image(const cv::Mat & mat) : filename("") {
   if (mat.data == NULL) {
      throw std::invalid_argument("parameter is null");
   }

   image = mat;
   valid = true;

   VLOG(5) << "Image::Image: width: " << image.cols
           << ", height: " << image.rows
           << ", depth: " << image.elemSize()
           << ", step: " << image.step1();
}

Image::Image(HANDLE h) : filename(""), handle(h) {
#ifdef WIN32
   BITMAPINFOHEADER *dibHeaderPtr = (BITMAPINFOHEADER *) GlobalLock(handle);

   // if these conditions are not met the Dib cannot be processed
   CHECK_EQ(dibHeaderPtr->biSize, 40);
   CHECK_EQ(dibHeaderPtr->biPlanes, 1);
   CHECK_EQ(dibHeaderPtr->biCompression, 0);
   CHECK_EQ(dibHeaderPtr->biXPelsPerMeter, dibHeaderPtr->biYPelsPerMeter);
   CHECK_EQ(dibHeaderPtr->biClrImportant, 0);

   unsigned rowBytes = static_cast<unsigned>(
      ceil((dibHeaderPtr->biWidth * dibHeaderPtr->biBitCount) / 32.0)) << 2;

   unsigned paletteSize;
   switch (dibHeaderPtr->biBitCount) {
      case 1:
         paletteSize = 2;
         break;
      case 4:
         paletteSize = 16;
         break;
      case 8:
         paletteSize = 256;
         break;
      default:
         paletteSize = 0;
   }

   char * pixels = reinterpret_cast <char *>(dibHeaderPtr)
      + sizeof(BITMAPINFOHEADER) + paletteSize * sizeof(RGBQUAD);

   int channels = 3; // RGB
   IplImage* cv_image = cvCreateImageHeader(
      cvSize(dibHeaderPtr->biWidth, dibHeaderPtr->biHeight),
      IPL_DEPTH_8U,
      channels);

   CHECK(cv_image != NULL) << "invalid image type";

   cvSetData(cv_image, pixels, cv_image->widthStep);
   cv::flip(cv::Mat(cv_image), image, 0);

   valid = true;
#else
   valid = false;
#endif
}


Image::~Image() {
   VLOG(5) << "Image destructor";
   image.release();

#ifdef WIN32
   if (handle != NULL) {
      GlobalUnlock(handle);
      GlobalFree(handle);
	  VLOG(5) << "Image handle freed";
   }
#endif
}

int Image::getWidth() const {
   return image.cols;
}

int Image::getHeight() const {
   return image.rows;
}

void Image::grayscale(Image & that) const {
    cv::cvtColor(image, that.image, cv::COLOR_BGR2GRAY);
}

// from: https://github.com/radeonwu/DMTag/blob/master/dm_localization/src/dm_localize.cpp
void Image::applyFilters(Image & that) const {
   cv::Mat blurredImage;
   cv::Mat enhancedImage;

   double sigma = 15, threshold = 5, amount = 1;

   cv::GaussianBlur(image, blurredImage, cv::Size(0, 0), sigma);
   cv::Mat lowContrastMask = abs(image - blurredImage) < threshold;
   that.image = image * (1 + amount) + blurredImage * (-amount);
   image.copyTo(that.image, lowContrastMask);
}

/**
 * Converts an OpenCV image to a DmtxImage.
 *
 * NOTE: The caller must detroy the image.
 */
DmtxImage * Image::dmtxImage() const {
   if (image.type() != CV_8UC1) {
      throw std::logic_error("invalid image type: " + image.type());
   }

   DmtxImage * dmtxImage = dmtxImageCreate(image.data, image.cols, image.rows, DmtxPack8bppK);
   return dmtxImage;
}

std::unique_ptr<const Image> Image::crop(unsigned x,
                                         unsigned y,
                                         unsigned width,
                                         unsigned height) const {
   cv::Rect roi(x, y, width, height);
    cv::Mat croppedRef(image, roi);
    cv::Mat cropped;
    croppedRef.copyTo(cropped);
    return std::unique_ptr<Image>(new Image(cropped));
}

void Image::drawRectangle(const cv::Rect & rect, const cv::Scalar & color) {
   cv::rectangle(image, rect, color);
}

void Image::drawLine(const cv::Point & pt1, const cv::Point & pt2, const cv::Scalar & color) {
   cv::line(image, pt1, pt2, color, 2);
}

int Image::write(const std::string & filename) const {
   VLOG(3) << "write: " << filename;
   int result = imwrite(filename.c_str(), image);
   return result;
}

}/* namespace */
