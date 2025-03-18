#!/usr/local/bin/python -OO
# Converts time in access_log from new format (microsecods) to old (seconds)

import sys;

inpFile = sys.stdin;
if len(sys.argv) >= 2:
    inpFile = open(sys.argv[1]);

for line in inpFile:
    fields = line.split(' ');
    for i in range(max(len(fields) - 9, 0), len(fields)):
        timeFields = fields[i].split('.');
        if len(timeFields) == 2 and timeFields[0].isdigit() and timeFields[1].isdigit():
            fields[i] = timeFields[0];
    print " ".join(fields),;
            
