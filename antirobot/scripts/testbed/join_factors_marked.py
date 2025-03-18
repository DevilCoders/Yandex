#!/usr/local/bin/python -OO

import sys;

def main():
    if len(sys.argv) < 3:
        print >>sys.stderr, "Usage: join_factors_marked.py <marked file> <dumped factors file>";
        sys.exit(1);

    markedFileName = sys.argv[1];
    dumpedFactorsFileName = sys.argv[2];

    mf = open(markedFileName);
    dff = open(dumpedFactorsFileName);

    reqidFromMf = "";

    for dumpedFactorsLine in dff:
        dumpedFactorsFields = dumpedFactorsLine.split('\t');
        reqidFromDff = dumpedFactorsFields[3];

        if len(reqidFromDff) < 17 or len(reqidFromDff.split('-')) < 2:
            continue;

        while not reqidFromMf.startswith(reqidFromDff):
            markedLine = mf.readline();
            if not markedLine:
                break;

            markedFields = markedLine.split('\t');
            reqidFromMf = markedFields[0].split(' ')[-2];

        if reqidFromMf == reqidFromDff:
            print "%s\t%s\t-\t0\t%s" % (reqidFromDff, markedFields[1].rstrip(),
                dumpedFactorsFields[4].rstrip()[1:-1].replace(',', '\t'))

main();

