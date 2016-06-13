#!/usr/bin/python
"""
rowAndColumnTurnDriveOff.py
 
  Usage demonstration of calling the functions in the KDK row and column
  driver shared library, after loading it dynamically.
  
  clears the continuous drive enable bit.
 
  Copyright 2015 Kymeta Corporation - All rights reserved.
 
  www.kymetacorp.com
  12277 134th Ct. NE, Suite 100
  Redmond, WA 98052
 
  Original code by Shawn Quinn
  squinn@kymetacorp.com
  Created Date:  06/12/2015
  
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
command = "ContinuousDriveEnable"
print "clearing continuous drive enable bit"
rowAndColDriverLib.clearRegisterBit(command)
print "unmapping memory"
rtnValue = rowAndColDriverLib.closeAndUnmapFpgaMemory()
if rtnValue == 0:
    print "munap memory function call successful..."
else:
    print "munap memory function call failed..."
