/*****************************************************************************
*
* defaultBoardConfigKDK.h
*
* Header file containing configuration values specific to the KDK controller
* board.  This file contains constant variable declarations.  These variables
* are used by library functions used to interface to the KDK board.
*
* Copyright 2015 Kymeta Corporation - All rights reserved.
*
* www.kymetacorp.com
* 12277 134th Ct. NE, Suite 100
* Redmond, WA 98052
*
* Original code by Shawn Quinn
* squinn@kymetacorp.com
* Created Date:  05/11/2015
*
*****************************************************************************/

#ifndef DEFAULTBOARDCONFIGKDK_H
#define DEFAULTBOARDCONFIGKDK_H

// define board specific parameters below
const uint8_t rowGroupSize = 10;
const int numRows = 105;
const int numCols = 158;

// declare non-board specific const variables
const int grayShades = 2;
const double m = 2.99792458;
const double pi = 3.1415926;
const double freqCoeff = 29.75;
const double indexOfRefraction = 1.565;
const double linearPolAngle = 45.0;
const double modPower = 1.0;
const double ePhiElement = 1.0;
const double eThetaElement = 1.0;

#endif
