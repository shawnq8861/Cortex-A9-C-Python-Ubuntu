#!/usr/bin/python
"""
writePatternBufferAllOn.py
 
  Usage demonstration of calling the functions in the KDK row and column
  driver shared library, after loading it dynamically.
  
  Pends on the isr value, then writes an all on pattern to the pattern buffer
 
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

print "populating modulation mask..."
rowAndColDriverLib.populateModulationMask()

sleep(1)
    
print "populating modulation matrix all on..."
pattern = "all on"
rowAndColDriverLib.populateModulationMatrix(pattern)

sleep(1)

print "\ntoggling hps bank select...\n"

addressOffset = c_uint8(0)
readValue = c_uint32(0)
writeValue = c_uint32(0)

# bank_sel_hps
addressOffset = 36
readValue = rowAndColDriverLib.readRegisterValue(addressOffset)
writeValue = ~readValue
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
sleep(1)

print "pending for irq assertion...\n"
# conifer_isr
addressOffset = 32
readValue = rowAndColDriverLib.readRegisterValue(addressOffset)
while readValue > 0:
    sleep(1)
    readValue = rowAndColDriverLib.readRegisterValue(addressOffset)

print "writing formatted data to pattern buffer..."
rowAndColDriverLib.formatAndWriteModulationToFPGAKDKFromScratch()
sleep(1)

print "\ntoggling conifer bank select...\n"
# bank_sel_conifer
addressOffset = 40
readValue = rowAndColDriverLib.readRegisterValue(addressOffset)
writeValue = ~readValue
rowAndColDriverLib.writeRegisterValue(addressOffset, writeValue)
sleep(1)

print "unmapping memory"
rtnValue = rowAndColDriverLib.closeAndUnmapFpgaMemory()
if rtnValue == 0:
    print "munap memory function call successful..."
else:
    print "munap memory function call failed..."
