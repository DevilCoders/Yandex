#!/usr/bin/env python

# Extracts Unicode symbol width table for Windows Arial font
# * Requires TTX/FontTools https://github.com/behdad/fonttools/
# * Reads files arial.ttf, arialbd.ttf, arialuni.ttf from
#   current directory (these files can be taken from
#   'C:\Windows\Fonts' directory on any Windows machine)
# * Writes arial_bold.inc, arial_regular.inc to current directory
# * The internal font unit is used in the output table.
#   The table can be scaled to any font size:
#   width_in_px = width / unitsPerEm * font_size_in_px

import sys
from fontTools import ttLib

def getWidths(path):
    font = ttLib.TTFont(path)
    unitsPerEm = font['head'].unitsPerEm
    cmap = font['cmap'].getcmap(3, 1) # Get Unicode subtable
    glyphcodes = {}
    for code, name in cmap.cmap.items():
        glyphcodes[name] = code
    hmtx = font['hmtx']
    widths = {}
    for name, (width, lsb) in hmtx.metrics.items():
        if name in glyphcodes:
            widths[glyphcodes[name]] = width
    return unitsPerEm, widths

def merge(width1, width2, blankWidth):
    widths = []
    for i in xrange(65536):
        if i in width1:
            w = width1[i]
        elif i in width2:
            w = width2[i]
        else:
            w = blankWidth
        widths.append(w)
    return widths

def outResults(widths, path):
    f = open(path, 'w')
    for i in xrange(65536):
        w = widths[i]
        f.write("%4d,%c" % (w, '\n' if i % 16 == 15 else ' '))
    f.close()

unitsPerEmReg, widthsReg = getWidths('arial.ttf')
unitsPerEmBold, widthsBold = getWidths('arialbd.ttf')
unitsPerEmUni, widthsUni = getWidths('arialuni.ttf')
if unitsPerEmReg != unitsPerEmUni or unitsPerEmBold != unitsPerEmUni:
    print 'Error: unitsPerEm mismatch'
    sys.exit(1)
print 'unitsPerEm =', unitsPerEmReg
blankWidth = unitsPerEmReg * 3 // 4
outResults(merge(widthsReg, widthsUni, blankWidth), 'arial_regular.inc')
outResults(merge(widthsBold, widthsUni, blankWidth), 'arial_bold.inc')
