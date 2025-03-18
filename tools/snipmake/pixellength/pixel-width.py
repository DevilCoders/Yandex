#!/usr/bin/python
# coding: utf-8

"""
Easy converter from float pixel widths (old format)
to new ui16 fixed-point format
"""

import os.path

def calc_widths(fn):

    col_count = 16
    # this should be the same as PIXEL_FP_PRECISION in pixellength.h file
    PIXEL_FP_PRECISION = 200

    f = open(fn, 'r')
    ints = []
    for line in f:
        nums = line.split(',')
        for n in nums:
            n = n.strip()
            if len(n) == 0:
                continue
            n = float(n) * PIXEL_FP_PRECISION
            if str(n)[-2:] != '.0':
                print "Precision overflow, please increase PIXEL_FP_PRECISION: ", n
            ni = int(n + 0.001) # defeat roundup error
            if ni > 64000:
                print "Fixed point (ui16) overflow: ", ni
            ints.append(str(ni).rjust(5))
    f.close()
    # output can be the same
    fo = open(os.path.basename(fn), 'w')
    # do a pretty printing
    while len(ints):
        chunk = ints[:col_count]
        l = ", ".join(chunk)
        ints = ints[col_count:]
        if len(ints):
            l = l + ','
        fo.write(l + "\n")

    fo.close()

if __name__ == '__main__':
    d = 'float-pix/'
    calc_widths(d + 'pix_yand_body_bold.inc')
    calc_widths(d + 'pix_yand_body_regular.inc')
    calc_widths(d + 'pix_yand_title_bold.inc')
    calc_widths(d + 'pix_yand_title_regular.inc')
