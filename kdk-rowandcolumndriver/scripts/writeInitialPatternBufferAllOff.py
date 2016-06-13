#!/usr/bin/python
"""
writeInitialPatternBuffer.py
 
  Usage demonstration of calling the functions in the KDK row and column
  driver shared library, after loading it dynamically.
  
  Initializes the registerrs, sends all off pattern and sets the continuous 
  drive enable bit.
 
  Copyright 2015 Kymeta Corporation - All rights reserved.
 
  www.kymetacorp.com
  12277 134th Ct. NE, Suite 100
  Redmond, WA 98052
 
  Original code by Shawn Quinn
  squinn@kymetacorp.com
  Created Date:  06/19/2015
  
"""

from ctypes import *
import ctypes
import os
import sys
from time import *

rowAndColDriverLib = cdll.LoadLibrary('/opt/kymeta/lib/libRowAndColDriver.so')
print "library loaded"

print "mapping memory"
pathName = "/dev/aperture-control"
rtnValue = rowAndColDriverLib.openAndMapFpgaMemory(pathName)
if rtnValue == 0:
    print "map memory function call successful..."
else:
    print "map memory function call failed..."
    sys.exit(0)

sleep(1)

print "populating modulation mask..."
rowAndColDriverLib.populateModulationMask()

sleep(1)

print "populating modulation matrix..."
pattern = "all off"
rowAndColDriverLib.populateModulationMatrix(pattern)

#print "writing all zeros to pattern buffer..."
#rowAndColDriverLib.zeroInitializePatternBuffer()

addressOffset = c_uint8(0)
writeValue = c_uint32(0)

# conifer_isr - clear irq
print "clearing irq..."
addressOffset = 32
writeValue = 0
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)

print "writing control values to FPGA control registers..."
# set the other regs
# ctrl_reg
addressOffset = 0
writeValue = 0
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# stx_clk_match_val
addressOffset = 4
writeValue = 8
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# supply_switch_dly_match_val
addressOffset = 8
writeValue = 10
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# gate_dly_match_val
addressOffset = 12
writeValue = 119
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# total_shift_amt
addressOffset = 16
writeValue = 105
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# start_cycle_dly_match_val
addressOffset = 20
writeValue = 7200
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# version
addressOffset = 24
writeValue = 0
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# sck_match_val
addressOffset = 28
writeValue = 0
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)

# row_sel_0
addressOffset = 44
writeValue = 4294967295
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# row_sel_1
addressOffset = 48
writeValue = 4294967295
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# row_sel_2
addressOffset = 52
writeValue = 65535
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# row_sel_3
addressOffset = 56
writeValue = 4278190080
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# row_sel_4
addressOffset = 60
writeValue = 131071
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)

# wait_for_data_valid_match_val
addressOffset = 64
writeValue = 2399
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
# pattern_ram
addressOffset = 32768
writeValue = 0
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)

print "reading control value from FPGA control registers..."
# start_cycle_dly_match_val
addressOffset = 20
readValue = c_uint32(0)
readValue = rowAndColDriverLib.readRegisterValue(addressOffset)
print "\nstart_cycle_dly_match_val = ", readValue, "\n"

print "writing formatted data to pattern buffer..."
rowAndColDriverLib.formatAndWriteModulationToFPGAKDKFromScratch()
sleep(1)

command = "ContinuousDriveEnable"
print "setting continuous drive enable bit"
rowAndColDriverLib.setRegisterBit(command)
sleep(1)

print "unmapping memory"
rtnValue = rowAndColDriverLib.closeAndUnmapFpgaMemory()
if rtnValue == 0:
    print "munap memory function call successful..."
else:
    print "munap memory function call failed..."
