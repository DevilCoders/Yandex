#!/usr/bin/env python
# -*- coding: utf8 -*-

import sys
import math
import multiprocessing as mp

class Printer:
    def __init__(self, stream):
        self.__stream = stream
        self.__mutex = mp.RLock()
    def __call__(self, line):
        with self.__mutex:
            print >> self.__stream, line
            self.__stream.flush()
        
Print = Printer(sys.stdout)
PrintLog = Printer(sys.stderr)

def Map(nprocs, func, taskiterable, portion=None):
    pool = mp.Pool(nprocs)
    pool.map(func, taskiterable, portion if portion else int(math.ceil(len(taskiterable)/nprocs)))
    pool.close()
    pool.join()


if __name__ == "__main__":
    def test(t):
        PrintLog("log: " + str(t))
        Print("test0")
        Print(str(t))
        Print("test1")
    Map(100, test, xrange(0, 10000))
