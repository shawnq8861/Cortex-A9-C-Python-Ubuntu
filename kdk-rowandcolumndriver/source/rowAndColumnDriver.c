/*****************************************************************************
 *
 * rowAndColumnDriver.c
 *
 * Implementation file for a function library that allows mapping Cyclone V
 * SoC RAM into virtual memory, calculating a KDK radial feed modulation
 * matrix, mapping the matrix to a KDK radial feed FPGA pattern and writing
 * the pattern into FPGA RAM.
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
 ****************************************************************************/

#include "rowAndColumnDriver.h"
#include "defaultBoardConfigKDK.h"
#include "kdkActiveCellMask.h"
#include "kdkRowAndColBitMap.h"
#include "kdkActiveCellGeometry.h"

static uint8_t 	modulationBuffer[MAX_ROWS * MAX_COLS];
static uint8_t 	modulationInput[MAX_ROWS * MAX_COLS];
static uint8_t 	modulationMask[MAX_ROWS * MAX_COLS];
static uint8_t	waveModMask[MAX_ROWS * MAX_COLS];
static uint32_t zeroBuffer[BUF_SIZE];
static double 	modulationReal[ACTV_CELLS];
static uint32_t modulationWave[ACTV_CELLS];

// declare variables for use in mapping hardware registers into process space
int 				fdFpgaReg;			// file descriptor for FPGA registers
volatile uint8_t*	fpgaRegBaseAddrPtr;	// holds return value from mmap call
uint32_t* 			patternBuffer;		// pointer to start of FPGA RAM

/*****************************************************************************
*
* function printFPBufferToFile()
*
* This function prints a type double buffer as a matrix to a csv file
*
*****************************************************************************/
void printFPBufferToFile(double* buffer, char* filePath)
{
	int col, row;
	FILE * pFile;
	pFile = fopen(filePath, "w");
	for (col = 0 ; col < numCols ; col++)
	{
	   for(row = 0; row < numRows; row++)
	   {
		   fprintf(pFile, "%f,", buffer[col + row * numCols]);
	   }
	   fprintf(pFile, "\n");
	}
	fclose (pFile);
}

/*****************************************************************************
*
* function printIntMatrixToFile()
*
* This function prints a type uint8_t 2D buffer to a csv file
*
*****************************************************************************/
void printIntMatrixToFile(uint8_t* buffer, const char* filePath,
		int buffSize)
{
	uint16_t colCount;
	uint16_t rowCount;
	FILE * pFile;
	pFile = fopen(filePath, "w");
	for (rowCount = 0; rowCount <  numRows; ++rowCount) {
		for (colCount = 0; colCount < numCols; ++colCount) {
			fprintf(pFile, "%u,", buffer[rowCount * numCols + colCount]);
		}
		fprintf(pFile, "\n");
	}
	fclose (pFile);
}

/*****************************************************************************
*
* function printIntBufferToFile()
*
* this function prints a type uint32_t 1D buffer to a csv file as hex values
*
*****************************************************************************/
void printIntBufferToFile(uint32_t* buffer, const char* filePath,
		int buffSize)
{
	int i;
	FILE * pFile;
	pFile = fopen(filePath, "w");
	for (i=0 ; i < buffSize ; i++)
	{
		fprintf (pFile, "%08x", buffer[i]);
		fprintf(pFile, "\n");
	}
	fclose (pFile);
}

/*****************************************************************************
 *
 * Utility function, evaluates input integer and returns true if even, false
 * if odd.
 *
 ****************************************************************************/
bool isEven(uint16_t intValue)
{
	return ( (intValue % 2 == 0) ? true : false );
}

/*****************************************************************************
 *
 * FPGA control function, sets the bit described by the input string.
 *
 * Performs a read-modify-write sequence.  First reads the register, then
 * performs bitwise OR with the bit of choice, then writes the new value
 * back into the register.
 *
 ****************************************************************************/
void setRegisterBit(const char* controlBitName)
{
	uint8_t tempRegister = *fpgaRegBaseAddrPtr;
	uint8_t mask;
	if (!strcmp(controlBitName, "ContinuousDriveEnable")) {
		mask = 1;	// bit 0 set to 1, all other bits set to 0
	}
	tempRegister |= mask;
	*fpgaRegBaseAddrPtr = tempRegister;
}

/*****************************************************************************
 *
 * FPGA control function, clears the bit described by the input string.
 *
 * Performs a read-modify-write sequence.  First reads the register, then
 * performs bitwise OR with the bit of choice, then writes the new value
 * back into the register.
 *
 ****************************************************************************/
void clearRegisterBit(const char* controlBitName)
{
	uint8_t tempRegister = *fpgaRegBaseAddrPtr;
	uint8_t mask;
	if (!strcmp(controlBitName, "ContinuousDriveEnable")) {
		mask = 254;	// bit 0 set to 0, all other bits set to 1
	}
	tempRegister &= mask;
	*fpgaRegBaseAddrPtr = tempRegister;
}

/*****************************************************************************
 *
 * FPGA control function, writes a value to a control register.
 *
 * Performs a direct write sequence, writes "value" directly to the register
 * address defined by the offset value, addressOffset, from the base address
 * used in the call to mmap().
 *
 ****************************************************************************/
void writeRegisterValue(uint8_t addressOffset, uint32_t writeValue)
{
	//*(fpgaRegBaseAddrPtr + addressOffset) = writeValue;
	// cast the pointer to 8 bit value to a pointer to 32 bit value, then
	// set the value it points to to the value passed in
	*((uint32_t*)(fpgaRegBaseAddrPtr + addressOffset)) = writeValue;
}

/*****************************************************************************
 *
 * FPGA control function, reads a value from a control register.
 *
 * Returns the integer value at the address pointed to by the FPGA register
 * base address + the offset value passed in.
 *
 ****************************************************************************/
uint32_t readRegisterValue(uint8_t addressOffset)
{
	// cast the pointer to 8 bit value to a pointer to 32 bit value, return
	// the value it points to
	return *((uint32_t*)((fpgaRegBaseAddrPtr + addressOffset)));
}

/*****************************************************************************
 *
 * Map FPGA Ram into virtual memory space.
 *
 * Returns 0 on success, -1 on failure
 *
 ****************************************************************************/
int openAndMapFpgaMemory(const char* pathName)
{
	// call open to obtain a file descriptor into virtual memory space
	fdFpgaReg = open( pathName, O_RDWR);
	if ( fdFpgaReg == -1 ) {
		printf("Cannot open device file.\n");
		return -1;
	}

	// map numPages of hardware addresses into virtual memory beginning at
	// the FPGA register base address, to allow accessing FPGA registers
	// and pattern ram
	fpgaRegBaseAddrPtr = (volatile uint8_t*)mmap(NULL, pageSize * numPages,
			PROT_READ | PROT_WRITE, MAP_SHARED, fdFpgaReg, 0);

	if( fpgaRegBaseAddrPtr == MAP_FAILED ) {
		printf( "ERROR: mmap() FPGA register failed...\n" );
		close( fdFpgaReg );
		return -1;
	}

	patternBuffer = (uint32_t*)(fpgaRegBaseAddrPtr + 8 * pageSize);

	return 0;
}

/*****************************************************************************
 *
 * Unmap FPGA Ram from virtual memory space.
 *
 * Returns 0 on success, -1 on failure
 *
 ****************************************************************************/
int closeAndUnmapFpgaMemory(void)
{
	if( munmap( (void*)fpgaRegBaseAddrPtr, pageSize * numPages ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fdFpgaReg );
		return -1;
	}

	return 0;
}

/*****************************************************************************
 *
 * Build up the modulation masks from the bit mask header files, for each
 * element in the arrays in the header files, write a 1 into the corresponding
 * cell of the mask matrix.
 *
 ****************************************************************************/
void populateModulationMask(void) {
	// initialize the mask to all 0's
	memset(modulationMask, 0 , sizeof(modulationMask));
	uint16_t elemCount;
	// for each row, column pair, write a 1 into the mask matrix
	for (elemCount = 0; elemCount < ACTV_CELLS; ++elemCount) {
		modulationMask[kdkRowBitMask[elemCount] * numCols +
						 kdkColumnBitMask[elemCount]] = 1;
	}
	printIntMatrixToFile(modulationMask, "modMask.csv",
				sizeof(modulationMask));
}

/*****************************************************************************
 *
 * Build up the modulation array from either:
 * 	1.	manual on/off configuration by mapping even and odd cells
 * 	2.	wave equation
 *
 ****************************************************************************/
void populateModulationMatrix(const char* patternType)
{
	uint16_t evenCell = 0;
	uint16_t oddCell = 0;
	uint16_t otherCell = 0;
	if (!strcmp(patternType, "all on")) {
		evenCell = 1;
		oddCell = 1;
		otherCell = 1;
	}
	if (!strcmp(patternType, "all off")) {
		evenCell = 0;
		oddCell = 0;
		otherCell = 0;
	}
	if (!strcmp(patternType, "checkerboard")) {
		evenCell = 1;
		oddCell = 1;
		otherCell = 0;
	}
	/*
	 * 		Need to create case for wave equation
	 */
	uint16_t colCount;
	uint16_t rowCount;
	// if not "wave equation"
	if (strcmp(patternType, "wave equation")) {
		for (rowCount = 0; rowCount <  numRows; ++rowCount) {
			for (colCount = 0; colCount < numCols; ++colCount) {
				if (isEven(colCount) && isEven(rowCount)) {
					modulationBuffer[rowCount * numCols + colCount] =
							evenCell * modulationMask[rowCount * numCols 
													  + colCount];
				}
				else if (!isEven(colCount) && !isEven(rowCount)) {
					modulationBuffer[rowCount * numCols + colCount] =
							oddCell * modulationMask[rowCount * numCols 
													 + colCount];
				}
				else {
					modulationBuffer[rowCount * numCols + colCount] =
							otherCell * modulationMask[rowCount * numCols 
													   + colCount];
				}
			}
		}
	}
	else { 	// wave equation
		for (rowCount = 0; rowCount <  numRows; ++rowCount) {
			for (colCount = 0; colCount < numCols; ++colCount) {
				modulationBuffer[rowCount * numCols + colCount] =
						waveModMask[rowCount * numCols + colCount]
						* modulationMask[rowCount * numCols + colCount];
			}
		}
	}

	printIntMatrixToFile(modulationBuffer, "modBuffer.csv",
			sizeof(modulationBuffer));
}

/*****************************************************************************
 *
 * Create an initialized input modulation buffer from the raw modulation
 * buffer.
 *
 ****************************************************************************/
void transposeModulationBuffer(void)
{
	uint16_t rowCount;
	uint16_t colCount;

	for (rowCount = 0; rowCount <  numRows; ++rowCount) {
		for (colCount = 0; colCount < numCols; ++colCount) {
			modulationInput[colCount * numRows + rowCount] =
			modulationBuffer[rowCount * numCols + colCount];
		}
	}
}

/*****************************************************************************
 *
 * Write a value of zero into every cell of the memory pointed to by the
 * patternBuffer pointer patternBufferStart.
 *
 ****************************************************************************/
void zeroInitializePatternBuffer(void)
{
	memset((uint8_t*)patternBuffer, 0 , 8 * pageSize);
}

/*****************************************************************************
 *
 * Read the memory pointed to by the patternBuffer pointer patternBuffer and
 * save it to a .csv file named fileName.
 *
 ****************************************************************************/
void readAndSavePatternBuffer(const char* fileName)
{
	printIntBufferToFile(patternBuffer, fileName,
				numRows * rowGroupSize);
}

/*****************************************************************************
 *
 * Write a block of data to pattern Ram starting at blockStart, of length
 * length, with an offset from start of pattern Ram of offset.
 *
 ****************************************************************************/
void writeBlockMemoryToPatternBuffer(uint16_t* blockStart, uint16_t length,
		uint16_t offset)
{
	memcpy(patternBuffer + offset, blockStart, length);
}

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
void calcWaveModulation(double theta, double phi, double phase)
{
	double eTheta;
	double ePhi;
	double complex waveIn;
	double complex waveOutTheta;
	double complex waveOutPhi;
	double complex modulation;
	double waveOutAngle;
	double tempMaxVal = 0.0;
	double maxModVal = 0.0;
	double rho;
	double rhoRound;

	double freq = freqCoeff * pow(10.0, 9.0);	// frequency in GHz
	double c = m * pow(10.0, 8.0);          // speed of light
	double kf = 2.0 * pi * freq / c;      	// wave number in free space
	double ks = kf * indexOfRefraction;  // wave number in the substrate
	double lPar = linearPolAngle * (pi / 180.0);
	double thetaMag = cos(lPar);
	double phiMag = sin(lPar);

	theta *= (pi / 180.0);
	phi *= (pi / 180.0);
	phase *= (pi / 180.0);

	uint16_t loopCount;
	for (loopCount = 0; loopCount < ACTV_CELLS; ++ loopCount) {
		eTheta = (sin(phi + kdkRotGeoVal[loopCount]))
				* (1.0 / eThetaElement);
		ePhi = (cos(phi + kdkRotGeoVal[loopCount]))
				* (1.0 / ePhiElement);
		rho = sqrt(pow(kdkXGeoVal[loopCount], 2) +  pow(kdkYGeoVal[loopCount], 
				2));
		rhoRound = (floor(rho * 10000.0)) / 10000.0;
		waveIn = cos(ks * rhoRound) + sin(ks * rhoRound) * I;
		waveOutAngle = kf * (kdkXGeoVal[loopCount] * sin(theta) * cos(phi) +
							 kdkYGeoVal[loopCount] * sin(theta) * sin(phi));
		waveOutTheta = thetaMag * (cos(waveOutAngle) + sin(waveOutAngle) * I);
		waveOutPhi = phiMag * (cos(phase) + sin(phase) * I)
							* (cos(waveOutAngle) + sin(waveOutAngle) * I);
		modulation = waveIn * waveOutTheta * eTheta
				   + waveIn * waveOutPhi * ePhi;
		tempMaxVal = fabs(modulation);
		maxModVal = (maxModVal > tempMaxVal ? maxModVal : tempMaxVal);
		modulationReal[loopCount] = creal(modulation);
	}

	for (loopCount = 0; loopCount < ACTV_CELLS; ++ loopCount) {
		double modulationTemp = pow(((modulationReal[loopCount] + maxModVal)
										/ (2.0 * maxModVal)), modPower);
		modulationTemp *= (grayShades - 1);
		modulationWave[loopCount] = (uint32_t)(floor(modulationTemp
										/ (grayShades - 1)));

		// map to modulation array mask...

		// for each row, column pair, write a 1 into the mask matrix
		waveModMask[kdkRowBitMask[loopCount] * numCols +
					kdkColumnBitMask[loopCount]] = modulationWave[loopCount];
	}

	// output data for comparison with Python generated values...
	printIntBufferToFile((uint32_t*)waveModMask, "waveModulationMatrix.csv", 
			ACTV_CELLS);
}

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
void formatAndWriteModulationToFPGAKDKFromScratch(void)
{
	uint32_t zeroValue = 0;
	memset(zeroBuffer, zeroValue, BUF_SIZE);
	uint16_t rowCount;
	uint16_t colCount;
	uint8_t byteOffset;
	uint32_t mask;
	for (rowCount = 0; rowCount <  numRows; ++rowCount) {
		for (colCount = 0; colCount < numCols; ++colCount) {
			byteOffset = byteOffsetsByColumn[colCount];
			mask = 1 << (byteStartingBitValuesByColumn[colCount]
						+ bitShiftValueByColumn[colCount]);
			if (modulationBuffer[rowCount * numCols + colCount] == 1) {
				zeroBuffer[byteOffset + rowCount * rowGroupSize] |= mask;
			}
			else {
				zeroBuffer[byteOffset + rowCount * rowGroupSize] &= ~mask;
			}
		}
	}
	printIntBufferToFile(zeroBuffer, "desiredPatternBuffer.csv",
				numRows * rowGroupSize);
	int i;
	for (i = 0; i < BUF_SIZE; ++i) {
		patternBuffer[i] = zeroBuffer[i];
		usleep(25);
	}
	printIntBufferToFile(patternBuffer, "actualPatternBuffer.csv",
				numRows * rowGroupSize);
}
