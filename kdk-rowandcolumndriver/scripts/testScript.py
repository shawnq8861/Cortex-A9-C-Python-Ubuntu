#! /usr/bin/env python

from ctypes import *
import ctypes
import sys
import subprocess
from time import *

addressOffset = c_uint8(0)
writeValue = c_uint8(0)

print "initializing values 32 and 3..."

sleep(1)

addressOffset = 32
writeValue = 3

print "printing sum of 32 and 3..."

sleep(1)

sum = c_uint8(0)
sum = addressOffset + writeValue

print "sum = " + str(sum)
    
sys.exit(0)
