#!/usr/bin/python3
# Copyright (C) 2016-2021 Canonical
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import os
import sys
import fcntl
import array

from struct import *

# Linux ioctl numbers in Python
#  http://code.activestate.com/recipes/578225-linux-ioctl-numbers-in-python/

# constant for linux portability
_IOC_NRBITS = 8
_IOC_TYPEBITS = 8

# architecture specific
_IOC_SIZEBITS = 14
_IOC_DIRBITS = 2

_IOC_NRMASK = (1 << _IOC_NRBITS) - 1
_IOC_TYPEMASK = (1 << _IOC_TYPEBITS) - 1
_IOC_SIZEMASK = (1 << _IOC_SIZEBITS) - 1
_IOC_DIRMASK = (1 << _IOC_DIRBITS) - 1

_IOC_NRSHIFT = 0
_IOC_TYPESHIFT = _IOC_NRSHIFT + _IOC_NRBITS
_IOC_SIZESHIFT = _IOC_TYPESHIFT + _IOC_TYPEBITS
_IOC_DIRSHIFT = _IOC_SIZESHIFT + _IOC_SIZEBITS

_IOC_NONE = 0
_IOC_WRITE = 1
_IOC_READ = 2

def _IOC(dir, type, nr, size):
    if isinstance(size, str):
        size = struct.calcsize(size)
    return dir  << _IOC_DIRSHIFT  | \
           type << _IOC_TYPESHIFT | \
           nr   << _IOC_NRSHIFT   | \
           size << _IOC_SIZESHIFT

def _IO(type, nr): return _IOC(_IOC_NONE, type, nr, 0)
def _IOR(type, nr, size): return _IOC(_IOC_READ, type, nr, size)
def _IOW(type, nr, size): return _IOC(_IOC_WRITE, type, nr, size)
def _IOWR(type, nr, size): return _IOC(_IOC_READ | _IOC_WRITE, type, nr, size)
# end of Linux ioctl numbers in Python

fwdtSysFile = "/dev/fwdt"

def getIoNum(cmd):
    if cmd == 'vga':
        return _IOWR(ord('p'), 1, 1288)
    if cmd == 'io':
        return _IOWR(ord('p'), 2, 8)
    if cmd == 'memory':
        return _IOWR(ord('p'), 3, 16)
    if cmd == 'cmos':
        return _IOWR(ord('p'), 4, 6)
    if cmd == 'ec':
        return _IOWR(ord('p'), 5, 6)
    if cmd == 'acpi':
        return _IOWR(ord('p'), 6, 272)
    return -1

def cmosRead(addr):
    file = open(fwdtSysFile)
    buf = array.array('B', pack('HHBB', 1, 0, addr, 0))
    fcntl.ioctl(file, getIoNum('cmos'), buf, 1)
    file.close
    return buf[5]

def ioReadByte(addr):
    file = open(fwdtSysFile)
    buf = array.array('B', pack('HHHB', 1, 0, addr, 0))
    fcntl.ioctl(file, getIoNum('io'), buf, 1)
    file.close
    return buf[6]

def ioReadWord(addr):
    file = open(fwdtSysFile)
    buf = array.array('B', pack('HHHH', 3, 0, addr, 0))
    fcntl.ioctl(file, getIoNum('io'), buf, 1)
    file.close
    return buf[6] + (buf[7] << 8)

def ecCheck():
    present = False
    file = open(fwdtSysFile)
    buf = array.array('B', pack('HHBB', 4, 0, 0, 0))
    fcntl.ioctl(file, getIoNum('ec'), buf, 1)
    file.close
    if buf[0] + (buf[1] << 8) == 0:
        present = True
    return present

def ecRead(addr):
    file = open(fwdtSysFile)
    buf = array.array('B', pack('HHBB', 1, 0, addr, 0))
    fcntl.ioctl(file, getIoNum('ec'), buf, 1)
    file.close
    return buf[5]

def main():


#    print(cmosRead(0)) # this is a cmos read example
#    print(hex(ioReadWord(0x1830))) # this is an io read example

if __name__ == "__main__":
    main()
