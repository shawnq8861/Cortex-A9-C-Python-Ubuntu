/*****************************************************************************
*
* rowAndColumnDriver.h
*
* Header file defining the interface for a function library that allows
* mapping Cyclone V SoC RAM into virtual memory, calculating a KDK radial feed
* modulation matrix, mapping the matrix to a KDK radial feed FPGA pattern and
* writing the pattern into FPGA RAM.
*
* This file contains preprocessor directives, library include statements, and
* function prototypes.
*
* Copyright 2015 Kymeta Corporation - All rights reserved.
*
* www.kymetacorp.com
* 12277 134th Ct. NE, Suite 100
* Redmond, WA 98052
*
* Original code by Shawn Quinn
* squinn@kymetacorp.com
* Created Date:  05/21/2015
*
*****************************************************************************/

#ifndef ROWANDCOLUMNDRIVER_H
#define ROWANDCOLUMNDRIVER_H

#include <sys/mman.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <complex.h>

// define the static shared array dimensions
#define	MAX_ROWS	160
#define MAX_COLS	160
#define BUF_SIZE	1050
#define ACTV_CELLS	8208

// declare numerical constants
const int pageSize = 4096;
const int numPages = 16;
const int pattRamOffset = 32768;

/*****************************************************************************
 *
 * Utility function, evaluates input integer and returns true if even, false
 * if odd.
 *
 ****************************************************************************/
bool isEven(uint16_t intValue);

/*****************************************************************************
 *
 * FPGA control function, sets the bit described by the input string.
 *
 * supported arguments when calling:		"ContinuousDriveEnable"
 *
 ****************************************************************************/
void setRegisterBit(const char* controlBitName);

/*****************************************************************************
 *
 * FPGA control function, clears the bit described by the input string.
 *
 * supported when calling arguments:		"ContinuousDriveEnable"
 *
 ****************************************************************************/
void clearRegisterBit(const char* controlBitName);

/*****************************************************************************
 *
 * FPGA control function, writes a value to a control register.
 *
 * Performs a direct write sequence, writes "value" directly to the register
 * address defined by the offset value, addressOffset, from the base address
 * used in the call to mmap().
 *
 ****************************************************************************/
void writeRegisterValue(uint8_t addressOffset, uint32_t writeValue);

/*****************************************************************************
 *
 * FPGA control function, reads a value from a control register.
 *
 * Returns the integer value at the address pointed to by the FPGA register
 * base address + the offset value passed in.
 *
 ****************************************************************************/
uint32_t readRegisterValue(uint8_t addressOffset);

/*****************************************************************************
 *
 * Map FPGA Ram into virtual memory space.
 *
 * Returns 0 on success, -1 on failure
 *
 ****************************************************************************/
int openAndMapFpgaMemory(const char* pathName);

/*****************************************************************************
 *
 * Unmap FPGA Ram from virtual memory space.
 *
 * Returns 0 on success, -1 on failure
 *
 ****************************************************************************/
int closeAndUnmapFpgaMemory(void);

/*****************************************************************************
 *
 * Build up the modulation masks from the bit mask header files, for each
 * element in the arrays in the header files, write a 1 into the corresponding
 * cell of the mask matrix.
 *
 ****************************************************************************/
void populateModulationMask(void);

/*****************************************************************************
 *
 * Build up the modulation array from either:
 * 	1.	manual on/off configuration by mapping even and odd cells
 * 	2.	wave equation
 *
 *  supported arguments when calling:	"all on"
 *	 	 	  							"all off"
 * 										"checkerboard"
 *
 ****************************************************************************/
void populateModulationMatrix(const char* patternType);

/*****************************************************************************
 *
 * Create an initialized input modulation buffer from the raw modulation
 * buffer.
 *
 ****************************************************************************/
void transposeModulationBuffer(void);

/*****************************************************************************
 *
 * Write a value of zero into every cell of the memory pointed to by the
 * patternBuffer pointer patternBuffer.
 *
 ****************************************************************************/
void zeroInitializePatternBuffer(void);

/*****************************************************************************
 *
 * Read the memory pointed to by the patternBuffer pointer patternBuffer and
 * save it to a .csv file named fileName.
 *
 ****************************************************************************/
void readAndSavePatternBuffer(const char* fileName);

/*****************************************************************************
 *
 * Write a block of data to pattern Ran starting at blockStart, of length
 * length, with an offset from blockStart of offset.
 *
 * argument1:	pointer to start of block of data
 * argument2:	length of data block in bytes
 * argument3:	byte offset from start of pattern buffer to first byte
 * 				of copied data
 *
 ****************************************************************************/
void writeBlockMemoryToPatternBuffer(uint16_t* blockStart, uint16_t length,
		uint16_t offset);

/*****************************************************************************
*
* function calcWaveModulation()
*
* - calculate the modulation matrix using the wave equation and hardware
*   parameters
* - calculations follow those implemented in:
* 		mtennaLib->Viking2.py->calculate_modulation()
* - arguments:	3 angles normally determined by antenna control algorithms
* - returns:	none
*
*****************************************************************************/
void calcWaveModulation(double theta, double phi, double phase);

/*****************************************************************************
 *
 * Re-code from scratch... using the spreadsheet as guide
 *
 * assume the pattern buffer has been zero initialized
 * placement algorithm repeats for each row
 *
 * the bit to set is either byte1 plus offset or byte3 + offset
 * byte0 and byte2 are unused and stay all 0
 * so:
 * 		start of byte1 == 8, start of byte3 == 24
 *
 *	the pattern buffer index value is byte offset + row * 10
 *
 * 	for each row
 * 		set each word value to zero
 * 		for each column
 * 			get byte and offset value from array in include header
 * 			if modulation [row][col] == 1
 * 				index into the correct word in the pattern buffer
 *				set bit correspond to bit position = byte value + offset
 *
 ****************************************************************************/
void formatAndWriteModulationToFPGAKDKFromScratch(void);

/*****************************************************************************
*
* function calcWaveModualtion()
*
* - calculate the modulation matrix using the wave equation and hardware
*   parameters
* - basis for the calculations came from the calculate_modulation() function
*   in the Viking2.py file which is part of the mtennaLib repository.
* - arguments:	theta (azimuth), phi (elevation), and pahes angles, values
* 				typically supplied from acquisition and control software
* - returns:	none
*
*****************************************************************************/
void calcWaveModulation(double theta, double phi, double phase);

#endif
