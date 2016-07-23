/*
 A (very) simple UI lib built on top of OpenCV drawing primitives.

 Version: 1.0.0

 Copyright (c) 2016 Fernando Bevilacqua <dovyski@gmail.com>
 Licensed under the MIT license.
*/

#include <iostream>

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "cvui.h"

namespace cvui
{

// This is an internal namespace with all code
// that is shared among components/functions
namespace internal {
	// Find the min and max values of a vector
	void findMinMax(std::vector<double>& theValues, double *theMin, double *theMax) {
		std::vector<double>::size_type aSize = theValues.size(), i;
		double aMin = theValues[0], aMax = theValues[0];

		for (i = 0; i < aSize; i++) {
			if (theValues[i] < aMin) {
				aMin = theValues[i];
			}

			if (theValues[i] > aMax) {
				aMax = theValues[i];
			}
		}

		*theMin = aMin;
		*theMax = aMax;
	}

	void separateChannels(int *theRed, int *theGreen, int *theBlue, unsigned int theColor) {
		*theRed = (theColor >> 16) & 0xff;
		*theGreen = (theColor >> 8) & 0xff;
		*theBlue = theColor & 0xff;
	}
}

// This is an internal namespace with all functions
// that actually render each one of the UI components
namespace render {
	const int IDLE = 0;
	const int OVER = 1;
	const int PRESSED = 2;

	void text(cv::Mat& theWhere, const cv::String& theText, cv::Point& thePos, double theFontScale, unsigned int theColor) {
		int aRed, aGreen, aBlue;

		aRed = (theColor >> 16) & 0xff;
		aGreen = (theColor >> 8) & 0xff;
		aBlue = theColor & 0xff;

		cv::putText(theWhere, theText, thePos, cv::FONT_HERSHEY_SIMPLEX, theFontScale, cv::Scalar(aBlue, aGreen, aRed), 1, cv::LINE_AA);
	}

	void button(int theState, cv::Mat& theWhere, cv::Rect& theShape, const cv::String& theLabel) {
		// Outline
		cv::rectangle(theWhere, theShape, cv::Scalar(0x29, 0x29, 0x29));

		// Border
		theShape.x++; theShape.y++; theShape.width -= 2; theShape.height -= 2;
		cv::rectangle(theWhere, theShape, cv::Scalar(0x4A, 0x4A, 0x4A));

		// Inside
		theShape.x++; theShape.y++; theShape.width -= 2; theShape.height -= 2;
		cv::rectangle(theWhere, theShape, theState == IDLE ? cv::Scalar(0x42, 0x42, 0x42) : (theState == OVER ? cv::Scalar(0x52, 0x52, 0x52) : cv::Scalar(0x32, 0x32, 0x32)), cv::FILLED);
	}

	void buttonLabel(int theState, cv::Mat& theWhere, cv::Rect theRect, const cv::String& theLabel, cv::Size& theTextSize) {
		cv::Point aPos(theRect.x + theRect.width / 2 - theTextSize.width / 2, theRect.y + theRect.height / 2 + theTextSize.height / 2);
		cv::putText(theWhere, theLabel, aPos, cv::FONT_HERSHEY_SIMPLEX, theState == PRESSED ? 0.39 : 0.4, cv::Scalar(0xCE, 0xCE, 0xCE), 1, cv::LINE_AA);
	}

	void counter(cv::Mat& theWhere, cv::Rect& theShape, const cv::String& theValue) {
		int aBaseline = 0;

		cv::rectangle(theWhere, theShape, cv::Scalar(0x29, 0x29, 0x29), cv::FILLED); // fill
		cv::rectangle(theWhere, theShape, cv::Scalar(0x45, 0x45, 0x45)); // border

		cv::Size aTextSize = getTextSize(theValue, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &aBaseline);

		cv::Point aPos(theShape.x + theShape.width / 2 - aTextSize.width / 2, theShape.y + aTextSize.height / 2 + theShape.height / 2);
		cv::putText(theWhere, theValue, aPos, cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0xCE, 0xCE, 0xCE), 1, cv::LINE_AA);
	}

	void checkbox(int theState, cv::Mat& theWhere, cv::Rect& theShape) {
		// Outline
		cv::rectangle(theWhere, theShape, theState == IDLE ? cv::Scalar(0x63, 0x63, 0x63) : cv::Scalar(0x80, 0x80, 0x80));

		// Border
		theShape.x++; theShape.y++; theShape.width -= 2; theShape.height -= 2;
		cv::rectangle(theWhere, theShape, cv::Scalar(0x17, 0x17, 0x17));

		// Inside
		theShape.x++; theShape.y++; theShape.width -= 2; theShape.height -= 2;
		cv::rectangle(theWhere, theShape, cv::Scalar(0x29, 0x29, 0x29), cv::FILLED);
	}

	void checkboxLabel(cv::Mat& theWhere, cv::Rect& theRect, const cv::String& theLabel, cv::Size& theTextSize, unsigned int theColor) {
		cv::Point aPos(theRect.x + theRect.width + 8, theRect.y + theRect.height / 2 + theTextSize.height / 2 + 2);
		text(theWhere, theLabel, aPos, 0.4, theColor);
	}

	void checkboxCheck(cv::Mat& theWhere, cv::Rect& theShape) {
		theShape.x++; theShape.y++; theShape.width -= 2; theShape.height -= 2;
		cv::rectangle(theWhere, theShape, cv::Scalar(0xFF, 0xBF, 0x75), cv::FILLED);
	}

	void window(cv::Mat& theWhere, cv::Rect& theTitleBar, cv::Rect& theContent, const cv::String& theTitle) {
		bool aTransparecy = false;
		double aAlpha = 0.3;
		cv::Mat aOverlay;

		// Render the title bar.
		// First the border
		cv::rectangle(theWhere, theTitleBar, cv::Scalar(0x4A, 0x4A, 0x4A));
		// then the inside
		theTitleBar.x++; theTitleBar.y++; theTitleBar.width -= 2; theTitleBar.height -= 2;
		cv::rectangle(theWhere, theTitleBar, cv::Scalar(0x21, 0x21, 0x21), cv::FILLED);

		// Render title text.
		cv::Point aPos(theTitleBar.x + 5, theTitleBar.y + 12);
		cv::putText(theWhere, theTitle, aPos, cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0xCE, 0xCE, 0xCE), 1, cv::LINE_AA);

		// Render the body.
		// First the border.
		cv::rectangle(theWhere, theContent, cv::Scalar(0x4A, 0x4A, 0x4A));

		// Then the filling.
		theContent.x++; theContent.y++; theContent.width -= 2; theContent.height -= 2;
		cv::rectangle(aOverlay, theContent, cv::Scalar(0x31, 0x31, 0x31), cv::FILLED);

		if (aTransparecy) {
			theWhere.copyTo(aOverlay);
			cv::rectangle(aOverlay, theContent, cv::Scalar(0x31, 0x31, 0x31), cv::FILLED);
			cv::addWeighted(aOverlay, aAlpha, theWhere, 1.0 - aAlpha, 0.0, theWhere);

		} else {
			cv::rectangle(theWhere, theContent, cv::Scalar(0x31, 0x31, 0x31), cv::FILLED);
		}
	}

	void rect(cv::Mat& theWhere, cv::Rect& thePos, unsigned int theColor) {
		int aRed, aGreen, aBlue;

		aRed = (theColor >> 16) & 0xff;
		aGreen = (theColor >> 8) & 0xff;
		aBlue = theColor & 0xff;

		cv::rectangle(theWhere, thePos, cv::Scalar(aBlue, aGreen, aRed), cv::FILLED, cv::LINE_AA);
	}

	void sparkline(cv::Mat& theWhere, std::vector<double> theValues, cv::Rect &theRect, double theMin, double theMax, unsigned int theColor) {
		std::vector<double>::size_type aSize = theValues.size(), i;
		double aGap, aPosX, aScale = 0, x, y;
		int aRed, aGreen, aBlue ;

		internal::separateChannels(&aRed, &aGreen, &aBlue, theColor);

		aScale = theMax - theMin;
		aGap = (double)theRect.width / aSize;
		aPosX = theRect.x;

		for (i = 0; i <= aSize - 2; i++) {
			x = aPosX;
			y = (theValues[i] - theMin) / aScale * -(theRect.height - 5) + theRect.y + theRect.height - 5;
			cv::Point aPoint1((int)x, (int)y);

			x = aPosX + aGap;
			y = (theValues[i + 1] - theMin) / aScale * -(theRect.height - 5) + theRect.y + theRect.height - 5;
			cv::Point aPoint2((int)x, (int)y);

			cv::line(theWhere, aPoint1, aPoint2, cv::Scalar(aBlue, aGreen, aRed));
			aPosX += aGap;
		}
	}
}

const int TYPE_ROW = 0;

typedef struct {
	cv::Mat where;
	cv::Rect rect;
	int type;
} cvui_block_t;

// Variables to keep track of mouse events and stuff
static bool gMouseJustReleased = false;
static bool gMousePressed = false;
static cv::Point gMouse;
static char gBuffer[1024];

static cvui_block_t gStack[100]; // TODO: make it dynamic?
static int gStackCount = -1;
	
void init(const cv::String& theWindowName) {
	cv::setMouseCallback(theWindowName, handleMouse, NULL);
}

bool button(cv::Mat& theWhere, int theX, int theY, const cv::String& theLabel) {
	// Calculate the space that the label will fill
	int aBaseline = 0;
	cv::Size aTextSize = getTextSize(theLabel, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &aBaseline);

	// Create a button based on the size of the text
	return button(theWhere, theX, theY, aTextSize.width + 30, aTextSize.height + 18, theLabel);
}

bool button(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, const cv::String& theLabel) {
	// Calculate the space that the label will fill
	int aBaseline = 0;
	cv::Size aTextSize = getTextSize(theLabel, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &aBaseline);

	// Make the button bit enough to house the label
	cv::Rect aRect(theX, theY, theWidth, theHeight);

	// Check the state of the button (idle, pressed, etc.)
	bool aMouseIsOver = aRect.contains(gMouse);

	if (aMouseIsOver) {
		if (gMousePressed) {
			render::button(render::PRESSED, theWhere, aRect, theLabel);
			render::buttonLabel(render::PRESSED, theWhere, aRect, theLabel, aTextSize);
		} else {
			render::button(render::OVER, theWhere, aRect, theLabel);
			render::buttonLabel(render::OVER, theWhere, aRect, theLabel, aTextSize);
		}
	} else {
		render::button(render::IDLE, theWhere, aRect, theLabel);
		render::buttonLabel(render::IDLE, theWhere, aRect, theLabel, aTextSize);
	}

	// Tell if the button was clicked or not
	return aMouseIsOver && gMouseJustReleased;
}

bool checkbox(cv::Mat& theWhere, int theX, int theY, const cv::String& theLabel, bool *theState, unsigned int theColor) {
	int aBaseline = 0;
	cv::Rect aRect(theX, theY, 15, 15);
	cv::Size aTextSize = getTextSize(theLabel, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &aBaseline);
	cv::Rect aHitArea(theX, theY, aRect.width + aTextSize.width, aRect.height);
	bool aMouseIsOver = aHitArea.contains(gMouse);

	if (aMouseIsOver) {
		render::checkbox(render::OVER, theWhere, aRect);

		if (gMouseJustReleased) {
			*theState = !(*theState);
		}
	} else {
		render::checkbox(render::IDLE, theWhere, aRect);
	}

	render::checkboxLabel(theWhere, aRect, theLabel, aTextSize, theColor);

	if (*theState) {
		render::checkboxCheck(theWhere, aRect);
	}

	return *theState;
}

void text(cv::Mat& theWhere, int theX, int theY, const cv::String& theText, double theFontScale, unsigned int theColor) {
	cv::Point aPos(theX, theY);
	render::text(theWhere, theText, aPos, theFontScale, theColor);
}

void printf(cv::Mat& theWhere, int theX, int theY, double theFontScale, unsigned int theColor, char *theFmt, ...) {
	cv::Point aPos(theX, theY);
	va_list aArgs;

	va_start(aArgs, theFmt);
	vsprintf_s(gBuffer, theFmt, aArgs);
	va_end(aArgs);

	render::text(theWhere, gBuffer, aPos, theFontScale, theColor);
}

int counter(cv::Mat& theWhere, int theX, int theY, int *theValue, int theStep, const char *theFormat) {
	cv::Rect aContentArea(theX + 22, theY + 1, 48, 21);

	if (cvui::button(theWhere, theX, theY, 22, 22, "-")) {
		*theValue -= theStep;
	}
	
	sprintf_s(gBuffer, theFormat, *theValue);
	render::counter(theWhere, aContentArea, gBuffer);

	if (cvui::button(theWhere, aContentArea.x + aContentArea.width, theY, 22, 22, "+")) {
		*theValue += theStep;
	}

	return *theValue;
}

double counter(cv::Mat& theWhere, int theX, int theY, double *theValue, double theStep, const char *theFormat) {
	cv::Rect aContentArea(theX + 22, theY + 1, 48, 21);

	if (cvui::button(theWhere, theX, theY, 22, 22, "-")) {
		*theValue -= theStep;
	}

	sprintf_s(gBuffer, theFormat, *theValue);
	render::counter(theWhere, aContentArea, gBuffer);

	if (cvui::button(theWhere, aContentArea.x + aContentArea.width, theY, 22, 22, "+")) {
		*theValue += theStep;
	}

	return *theValue;
}

void window(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, const cv::String& theTitle) {
	cv::Rect aTitleBar(theX, theY, theWidth, 20);
	cv::Rect aContent(theX, theY + aTitleBar.height, theWidth, theHeight - aTitleBar.height);

	render::window(theWhere, aTitleBar, aContent, theTitle);
}

void rect(cv::Mat& theWhere, int theX, int theY, int theWidth, int theHeight, unsigned int theColor) {
	cv::Rect aRect(theX, theY, theWidth, theHeight);
	render::rect(theWhere, aRect, theColor);
}

void sparkline(cv::Mat& theWhere, std::vector<double> theValues, int theX, int theY, int theWidth, int theHeight, unsigned int theColor) {
	double aMin, aMax;
	cv::Rect aRect(theX, theY, theWidth, theHeight);

	internal::findMinMax(theValues, &aMin, &aMax);
	render::sparkline(theWhere, theValues, aRect, aMin, aMax, theColor);
}

void sparklineChart(cv::Mat& theWhere, std::vector<double> theValues, int theX, int theY, int theWidth, int theHeight) {
	double aMin, aMax, aScale = 0;

	internal::findMinMax(theValues, &aMin, &aMax);
	aScale = aMax - aMin;

	sparkline(theWhere, theValues, theX, theY, theWidth, theHeight);

	cvui::printf(theWhere, theX + 2, theY + 8, 0.25, 0x717171, "%.1f", aMax);
	cvui::printf(theWhere, theX + 2, theY + theHeight / 2, 0.25, 0x717171, "%.1f", aScale / 2 + aMin);
	cvui::printf(theWhere, theX + 2, theY + theHeight - 5, 0.25, 0x717171, "%.1f", aMin);
}

void beginRow(cv::Mat &theWhere, int theX, int theY, int theWidth, int theHeight) {
	// TODO: move this to internal namespace?
	cvui_block_t& aBlock = gStack[++gStackCount];
	
	aBlock.where = theWhere;
	aBlock.rect.x = theX;
	aBlock.rect.y = theY;
	aBlock.rect.width = theWidth;
	aBlock.rect.height = theHeight;
	aBlock.type = TYPE_ROW;
}

void endRow() {
	// TODO: check for empty stack before getting things out of it.
	gStackCount--;
}

void text(const cv::String& theText, double theFontScale, unsigned int theColor) {
	cvui_block_t& aBlock = gStack[gStackCount];

	cv::Point aPos(aBlock.rect.x, aBlock.rect.y);
	render::text(aBlock.where, theText, aPos, theFontScale, theColor);

	// TODO: calculate the real width of the text
	aBlock.rect.x += 50;
}

void update() {
	gMouseJustReleased = false;
}

void handleMouse(int theEvent, int theX, int theY, int theFlags, void* theData) {
	gMouse.x = theX;
	gMouse.y = theY;

	if (theEvent == cv::EVENT_LBUTTONDOWN || theEvent == cv::EVENT_RBUTTONDOWN)	{
		gMousePressed = true;

	} else if (theEvent == cv::EVENT_LBUTTONUP || theEvent == cv::EVENT_RBUTTONUP)	{
		gMouseJustReleased = true;
		gMousePressed = false;
	}
}

} // namespace cvui