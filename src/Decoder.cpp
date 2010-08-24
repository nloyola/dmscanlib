/*
Dmscanlib is a software library and standalone application that scans 
and decodes libdmtx compatible test-tubes. It is currently designed 
to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
Copyright (C) 2010 Canadian Biosample Repository

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * Decoder.cpp
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

#include "Decoder.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Dib.h"
#include "Util.h"
#include "BarcodeInfo.h"
#include "BinRegion.h"
#include "ProcessImageManager.h"


#include <time.h>
#include <iostream>
#include <math.h>
#include <string>
#include <limits>
#include <vector>
#include <cmath>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

using namespace std;

Decoder::Decoder(double g, unsigned s, unsigned t, unsigned c, double dist, 
				 double gx, double gy,
				 unsigned profileA,unsigned profileB, unsigned profileC)
				 : profile(profileA,profileB,profileC)
{
	ua::Logger::Instance().subSysHeaderSet(3, "Decoder");
	scanGap = g;
	squareDev = s;
	edgeThresh = t;
	corrections = c;
	imageBuf = NULL;
	cellDistance = dist;
	gapX = gx;
	gapY = gy;

	UA_DOUT_NL(1,4,"Loaded Profile: ");
	for(int i=0; i < 96; i++){
		if(i % 12 == 0) {
			UA_DOUT_NL(1,4,"\n");
		}
		UA_DOUT_NL(1, 4, profile.isSetBit(i));
	}
	UA_DOUT_NL(1,4,"\n\n");
}

Decoder::~Decoder()
{
	BarcodeInfo *b;

	while (barcodeInfos.size() > 0) {
		b = barcodeInfos.back();
		barcodeInfos.pop_back();
		UA_ASSERT_NOT_NULL(b);
		delete b;
	}

	if (imageBuf != NULL) {
		free(imageBuf);
	}
}

void Decoder::initCells(unsigned maxRows, unsigned maxCols)
{
	cells.resize(maxRows);
	for (unsigned row = 0; row < maxRows; ++row) {
		cells[row].resize(maxCols);
	}
}

void Decoder::reduceBlobToMatrix(unsigned blobCount,Dib * dib, CvRect & inputBlob){
	Dib croppedDib;
	CBlobResult blobs;
	IplImageContainer *img;
	//float contrastSum, count, contrastRatio;

	croppedDib.crop(*dib,
		  inputBlob.x,
		  inputBlob.y,
		  inputBlob.x + inputBlob.width,
		  inputBlob.y + inputBlob.height);

	img = croppedDib.generateIplImage();
	
	for (int i = 0; i < 5; i++) 
		cvSmooth(img->getIplImage(), img->getIplImage(), CV_GAUSSIAN, 11, 11);
	cvThreshold(img->getIplImage(), img->getIplImage(), 50, 255, CV_THRESH_BINARY);

	blobs = CBlobResult(img->getIplImage(), NULL, 0);
	
	switch(img->getHorizontalResolution()){
		case 300:
			blobs.Filter(blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 840);
			break;
		case 400:
			blobs.Filter(blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 1900);
			break;
		case 600:
			blobs.Filter(blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 2400);
			break;
	}
	delete img;


	/* ---- Generates the value of relative contrast within the image -----*/
	/*
	count = 0;
	contrastSum = 0;
	contrastRatio = 0;
	for (int i = 0; i < (int)croppedDib.getHeight() - 1; i++) {
		for (int j =0; j < (int)croppedDib.getWidth() - 1; j++) {
			float center = (float)croppedDib.getPixelGrayscale(i,j);
			float right  = (float)croppedDib.getPixelGrayscale(i,j+1);
			float down  = (float)croppedDib.getPixelGrayscale(i+1,j);

			contrastSum +=  (float)((center-right)*(center-right) + (center-down)*(center-down));
			count++;
		}
	}
	contrastRatio = contrastSum / count;
	UA_DOUT(1, 9, "Contrast Ratio: " << contrastRatio );
	*/
	/* ---- Grabs the largest blob in the blobs vector -----*/
	bool reducedBlob = false;
	CvRect largestBlob;
	largestBlob.x =0;
	largestBlob.y =0;
	largestBlob.width = 0;
	largestBlob.height = 0;

	for (int i = 0; i < blobs.GetNumBlobs(); i++) {
		CvRect currentBlob = blobs.GetBlob(i)->GetBoundingBox();
		if(currentBlob.width*currentBlob.height > largestBlob.width*largestBlob.height){
			largestBlob = currentBlob;
			reducedBlob = true;
		}
	}	

	/* ---- Keep blobs that were successfully reduced-----*/
	if(reducedBlob){
		largestBlob.x += inputBlob.x;
		largestBlob.y += inputBlob.y;
		inputBlob = largestBlob;
	}
	else{
		inputBlob.x = 0;
		inputBlob.y = 0;
		inputBlob.width = 0;
		inputBlob.height = 0;
		UA_DOUT(1,1,"could not reduce blob #" << blobCount);
		return;
	}

}

struct BlobPosition{
	CvRect position;
	CvRect blob;
};

#define PALLET_COLUMNS 12
#define PALLET_ROWS 8

Decoder::ProcessResult Decoder::processImageRegions(Dib * dib, bool matrical)
{
	vector < CvRect > blobVector;
	vector<struct BlobPosition * > rectVector;

	RgbQuad green(0,255,0);
	RgbQuad yellow(255,255,0);

	double barcodeSizeInches = 0.13;
	double minBlobWidth =  ((double)dib->getDpi()*barcodeSizeInches);
	double minBlobHeight = ((double)dib->getDpi()*barcodeSizeInches);

	double dpi = (double)dib->getDpi();
	double w = dib->getWidth() / ((double)PALLET_COLUMNS);
	double h =  dib->getHeight() / ((double)PALLET_ROWS);

	UA_DOUT(1,7,"Minimum blob width (pixels): " << minBlobWidth);
	UA_DOUT(1,7,"Minimum blob height (pixels): " << minBlobHeight);

	for (int j = 0; j < PALLET_ROWS; j++) {
		for (int i = 0; i < PALLET_COLUMNS; i++) {
			
			/*
			The scanned image is a mirror image, 
			so we must flip our coordinates in the horizontal axis.
			*/
			unsigned position = (PALLET_COLUMNS-1)-i + j*PALLET_COLUMNS;

			if(!profile.isSetBit(position)){
				continue;
			}

			double cx = i * w + w / 2.0;
			double cy = j * h + h / 2.0;

			

			CvRect img;
			img.x = (int)(cx - w / 2.0 + (gapX*dpi) / 2.0);
			img.y = (int)(cy - h / 2.0 + (gapY*dpi) / 2.0);
			img.width = (int)( w - gapX*dpi);
			img.height = (int)(h - gapY*dpi);

			reduceBlobToMatrix(position,dib,img);

			if(img.width != 0 && img.height != 0 && 
				img.width >= minBlobWidth && img.height >= minBlobHeight){

				struct BlobPosition * pair = new struct BlobPosition();
				pair->position.x = (PALLET_COLUMNS-1) - i;
				pair->position.y = j;
				pair->blob = img;
				rectVector.push_back(pair);

				blobVector.push_back(img);
			}
		}
	}
	unsigned n,m,i,j;

	// Debug level >= 4 
	if(ua::Logger::Instance().isDebug(3, 4)){

		Dib blobDib(*dib);
		RgbQuad white(255,255,255);

		for ( i = 0, n = blobVector.size(); i < n; i++) {
			blobDib.rectangle(blobVector[i].x,blobVector[i].y,blobVector[i].width,blobVector[i].height,white);
		}
		blobDib.writeToFile("blobRegions.bmp");

		UA_DOUT(1,4,"Created blobRegion bitmap");
	}

	ProcessImageManager imageProcessor(this, scanGap, squareDev, edgeThresh, corrections);
	imageProcessor.generateBarcodes(dib, blobVector, barcodeInfos);

	if (barcodeInfos.empty()) {
		for ( j = 0, m = rectVector.size(); j < m; j++) 
			delete rectVector[j];

		UA_DOUT(3, 5, "no barcodes were found.");
		return IMG_INVALID;
	}

	width = dib->getWidth();
	height = dib->getHeight();

	initCells(PALLET_ROWS,PALLET_COLUMNS);

	for (i = 0, n = barcodeInfos.size(); i < n; ++i) {
		DmtxPixelLoc & tlCorner = barcodeInfos[i]->getTopLeftCorner();
		DmtxPixelLoc & brCorner = barcodeInfos[i]->getBotRightCorner();

		double avgx = (tlCorner.X + brCorner.X)/2.0;
		double avgy = (tlCorner.Y + brCorner.Y)/2.0;

		bool foundGrid = false;
		for ( j = 0, m = rectVector.size(); j < m; j++) {

			// pt inside rectangle
			if(avgx > rectVector[j]->blob.x && 
			   avgx < rectVector[j]->blob.x + rectVector[j]->blob.width &&
			   avgy > rectVector[j]->blob.y && 
			   avgy < rectVector[j]->blob.y + rectVector[j]->blob.height){
				    cells[ rectVector[j]->position.y ][ rectVector[j]->position.x  ] = barcodeInfos[i]->getMsg();
					foundGrid = true;
					break;
			}
		}
		if(!foundGrid){
			for ( j = 0, m = rectVector.size(); j < m; j++) 
				delete rectVector[j];
			
			return POS_CALC_ERROR;
		}
	}
	for ( j = 0, m = rectVector.size(); j < m; j++) 
		delete rectVector[j];
	

	return OK; 
}

DmtxImage * Decoder::createDmtxImageFromDib(Dib & dib)
{
	int pack = DmtxPackCustom;
	unsigned padding = dib.getRowPadBytes();

	switch (dib.getBitsPerPixel()) {
	case 8:
		pack = DmtxPack8bppK;
		break;
	case 24:
		pack = DmtxPack24bppRGB;
		break;
	case 32:
		pack = DmtxPack32bppXRGB;
		break;
	}

	DmtxImage *image = dmtxImageCreate(dib.getPixelBuffer(), dib.getWidth(),
					   dib.getHeight(), pack);

	//set the properties (pad bytes, flip)
	dmtxImageSetProp(image, DmtxPropRowPadBytes, padding);
	dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY);	// DIBs are flipped in Y
	return image;
}

void Decoder::showStats(DmtxDecode * dec, DmtxRegion * reg, DmtxMessage * msg)
{
	int height;
	int dataWordLength;
	int rotateInt;
	double rotate;
	DmtxVector2 p00, p10, p11, p01;

	height = dmtxDecodeGetProp(dec, DmtxPropHeight);

	p00.X = p00.Y = p10.Y = p01.X = 0.0;
	p10.X = p01.Y = p11.X = p11.Y = 1.0;
	dmtxMatrix3VMultiplyBy(&p00, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p10, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p11, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p01, reg->fit2raw);

	dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords,
						reg->sizeIdx);

	rotate = (2 * M_PI) + (atan2(reg->fit2raw[0][1], reg->fit2raw[1][1])
			       - atan2(reg->fit2raw[1][0],
				       reg->fit2raw[0][0])) / 2.0;

	rotateInt = (int)(rotate * 180 / M_PI + 0.5);
	if (rotateInt >= 360)
		rotateInt -= 360;

	fprintf(stdout, "--------------------------------------------------\n");
	fprintf(stdout, "       Matrix Size: %d x %d\n",
		dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx),
		dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx));
	fprintf(stdout, "    Data Codewords: %d (capacity %d)\n",
		dataWordLength - msg->padCount, dataWordLength);
	fprintf(stdout, "   Error Codewords: %d\n",
		dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords,
				       reg->sizeIdx));
	fprintf(stdout, "      Data Regions: %d x %d\n",
		dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions,
				       reg->sizeIdx),
		dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions,
				       reg->sizeIdx));
	fprintf(stdout, "Interleaved Blocks: %d\n",
		dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks,
				       reg->sizeIdx));
	fprintf(stdout, "    Rotation Angle: %d\n", rotateInt);
	fprintf(stdout, "          Corner 0: (%0.1f, %0.1f)\n", p00.X,
		height - 1 - p00.Y);
	fprintf(stdout, "          Corner 1: (%0.1f, %0.1f)\n", p10.X,
		height - 1 - p10.Y);
	fprintf(stdout, "          Corner 2: (%0.1f, %0.1f)\n", p11.X,
		height - 1 - p11.Y);
	fprintf(stdout, "          Corner 3: (%0.1f, %0.1f)\n", p01.X,
		height - 1 - p01.Y);
	fprintf(stdout, "--------------------------------------------------\n");
}

void Decoder::imageShowBarcodes(Dib & dib, bool regions)
{

	UA_DOUT(3, 3, "marking tags ");
	if(barcodeInfos.size() == 0)
		return;

	RgbQuad quadWhite(255, 255, 255);	// change to white (shows up better in grayscale)
	RgbQuad quadPink(255, 0, 255);
	RgbQuad quadRed(255, 0, 0);

	RgbQuad & highlightQuad =
	    (dib.getBitsPerPixel() == 8 ? quadWhite : quadPink);

	for (unsigned i = 0, n = barcodeInfos.size(); i < n; ++i) {
		BarcodeInfo & info = *barcodeInfos[i];
		DmtxPixelLoc & tlCorner = info.getTopLeftCorner();
		DmtxPixelLoc & brCorner = info.getBotRightCorner();

		dib.line(tlCorner.X, tlCorner.Y, tlCorner.X, brCorner.Y,
			 highlightQuad);
		dib.line(tlCorner.X, brCorner.Y, brCorner.X, brCorner.Y,
			 highlightQuad);
		dib.line(brCorner.X, brCorner.Y, brCorner.X, tlCorner.Y,
			 highlightQuad);
		dib.line(brCorner.X, tlCorner.Y, tlCorner.X, tlCorner.Y,
			 highlightQuad);
	}

	unsigned logLevel = ua::Logger::Instance().levelGet(3);

	if (logLevel == 0 || !regions)
		return;

	unsigned height = dib.getHeight() - 1;
	unsigned width = dib.getWidth() - 1;
}

// this method needs to be thread safe
BarcodeInfo * Decoder::addBarcodeInfo(DmtxDecode *dec, DmtxRegion *reg,
		DmtxMessage *msg) {
	addBarcodeMutex.lock();
	BarcodeInfo * info = NULL;

	string str((char *)msg->output, msg->outputIdx);

	if (barcodesMap[str] == NULL) {
		// this is a unique barcode
		info = new BarcodeInfo(dec, reg, msg);
		UA_ASSERT_NOT_NULL(info);
		barcodesMap[str] = info;
		barcodeInfos.push_back(info);
		UA_DOUT(3, 9, "Succesfully added barcode: " << info->getMsg());
	}

	addBarcodeMutex.unlock();
	return info;
}
