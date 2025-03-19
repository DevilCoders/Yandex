#!/usr/bin/env python

import sys
import os
import optparse
import random

#generates cpp charmap for cp1251 encoding

class out_iter:
    def __init__(self, start_cnt):
        #start_cnt can be 0 or 2 for special pseudo symbols 'S' and 'E'
        self.ifrom = 0
        self.ito = start_cnt
        self.charmap = dict()

    def print_next(self):
        self._do_print(self.ito)
        self.ito += 1

    def print_empty(self):
        self._do_print(255)

    def print_clone(self, clone_from):
        #for russian e -> yo, soft mark -> hard mark
        self._do_print(self.charmap[clone_from])

    def print_manual(self, manual_to):
        self._do_print(manual_to)

    def _do_print(self, ito):
        print '    {},'.format(ito)
        self.charmap[self.ifrom] = ito
        self.ifrom += 1



def main():
    parser = optparse.OptionParser("%prog [options]")
    (options, args) = parser.parse_args()

    print 'static const ui8 charmap[] = {'
    printer = out_iter(2)
    for ifrom in xrange(256):
        if chr(ifrom) == 'S':
            printer.print_manual(0)
        elif chr(ifrom) == 'E':
            printer.print_manual(1)
        elif (ifrom >= 48 and ifrom <= 57): # 0-9
            printer.print_next()
        elif (ifrom >= 97 and ifrom <= 122): # english abc
            printer.print_next()
        elif (ifrom == 184): # russian yo
            printer.print_next()
        elif (ifrom >= 224 and ifrom <= 255): # russian abc
            printer.print_next()
        elif (ifrom == 179):
            printer.print_clone(ord('i'))
        elif (ifrom == 188):
            printer.print_clone(ord('j'))
        elif (ifrom == 190):
            printer.print_clone(ord('s'))
        elif (ifrom == 189):
            printer.print_clone(ord('S'))
        else:
            printer.print_empty()
    print '};'


if __name__ == '__main__':
    main()
