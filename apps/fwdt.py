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

import os, sys, fcntl, array, argparse

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

class FWDT_Obj(object):
    def __init__(self):
        self.dev = '/dev/fwdt'
        self.sys = '/sys/bus/platform/devices/fwdt'

        if not os.path.exists(self.sys) or not os.path.exists(self.dev):
            sys.exit(-1)

    def write_sysfs(self, sysfs, data):
        f = open(sysfs, 'w')
        f.write(data)
        f.close()

    def read_sysfs(self, sysfs):
        f = open(sysfs, 'r')
        value = f.read()
        f.close()
        return value

class FWDT_SIMPLE(FWDT_Obj):
    def __init__(self):
        FWDT_Obj.__init__(self)

    def get_register(self, reg):
        self.write_sysfs(self.sys ,reg)
        f = open(self.sys, 'r')
        value = f.read()
        f.close()
        return value

class FWDT_IO(FWDT_Obj):
    def __init__(self, address, data):
        FWDT_Obj.__init__(self)
        self.address = os.path.join(self.sys, address)
        self.data = os.path.join(self.sys, data)

    def write_address(self, addr):
        self.write_sysfs(self.address, addr)

    def read_data(self):
        return self.read_sysfs(self.data)

    def write_data(self, data):
        self.write_sysfs(self.data, data)

class FWDT_MSR(FWDT_SIMPLE):
    def __init__(self):
        FWDT_Obj.__init__(self)
        self.sys = os.path.join(self.sys, 'msr')

class FWDT_CMOS(FWDT_SIMPLE):
    def __init__(self):
        FWDT_Obj.__init__(self)
        self.sys = os.path.join(self.sys, 'cmos')

class FWDT_PCI(FWDT_Obj):
    def __init__(self):
        FWDT_Obj.__init__(self)
        self.pci_id = os.path.join(self.sys, 'pci_id')
        self.pci_reg = os.path.join(self.sys, 'pci_reg')
        self.pci_data = os.path.join(self.sys, 'pci_data')

    def write_id(self, id):
        self.write_sysfs(self.pci_id, id)

    def write_reg(self, reg):
        self.write_sysfs(self.pci_reg, reg)

    def write_data(self, data):
        self.write_sysfs(self.pci_data, data)

    def read_data(self):
        return self.read_sysfs(self.pci_data)

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
    write_op = False
    parser = argparse.ArgumentParser(description='FWDT utility.')
    parser.add_argument("-c", "--cmos", help="Read CMOS registers")
    parser.add_argument("-e", "--ec", nargs='+', help="Read & Write EC registers")
    parser.add_argument("--iob", nargs='+', help="Read & Write I/O byte-access registers")
    parser.add_argument("--iow", nargs='+', help="Read & Write I/O word-access registers")
    parser.add_argument("-m", "--msr", help="Read MSR registers")
    parser.add_argument("-p", "--pci", nargs='+', help="Read & Write PCI registers")

    args = parser.parse_args()
    if args.msr:
        msr = FWDT_MSR()
        val = msr.get_register(args.msr)
    elif args.cmos:
        cmos = FWDT_CMOS()
        val = cmos.get_register(args.cmos)
    elif args.iob:
        iob = FWDT_IO('io_address', 'iob_data')
        if len(args.iob) == 1:
            ''' read from an I/O port '''
            iob.write_address(args.iob[0])
            val = iob.read_data()
        elif len(args.iob) == 2:
            ''' Write to an I/O port '''
            iob.write_address(args.iob[0])
            iob.write_data(args.iob[1])
            write_op = True
    elif args.iow:
        iow = FWDT_IO('io_address', 'iow_data')
        if len(args.iow) == 1:
            ''' read from an I/O port '''
            iow.write_address(args.iow[0])
            val = iow.read_data()
        elif len(args.iow) == 2:
            ''' Write to an I/O port '''
            iow.write_address(args.iow[0])
            iow.write_data(args.iow[1])
            write_op = True
    elif args.ec:
        ec = FWDT_IO('ec_address', 'ec_data')
        if len(args.ec) == 1:
            ''' read from EC '''
            ec.write_address(args.ec[0])
            val = ec.read_data()
        elif len(args.ec) == 2:
            ''' Write to EC '''
            ec.write_address(args.ec[0])
            ec.write_data(args.ec[1])
            write_op = True
    elif args.pci:
        pci = FWDT_PCI()
        if len(args.pci) == 2:
            ''' read from PCI '''
            pci.write_id(args.pci[0])
            pci.write_reg(args.pci[1])
            val = pci.read_data()
        elif len(args.pci) == 3:
            ''' Write to PCI '''
            pci.write_id(args.pci[0])
            pci.write_reg(args.pci[1])
            pci.write_data(args.pci[2])
            write_op = True


    if not write_op:
        print(val.strip())

if __name__ == "__main__":
    main()
