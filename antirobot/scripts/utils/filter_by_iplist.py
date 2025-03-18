#!/usr/bin/env python

import sys;
sys.path.append(".")
from optparse import OptionParser;
import ip_utils;

def main():
    usage = """usage: %prog [options] ip-range1 [ip-range2 .. ip-rangeN]
       %prog [options] ip_list_file"""

    parser = OptionParser(usage)
    parser.add_option("-v", dest="exclude", help="exclude mode", action="store_true")
    parser.add_option("-m", dest="mapreduce", help="mapreduce mode: will skip first tab delimited value", action="store_true")
    parser.add_option("-f", dest="ipListFile", help="read ip list from file", action="store_true")
    parser.add_option("-k", dest="field", help="field number to check", action="store")

    (options, args) = parser.parse_args()
    if len(args) == 0:
        parser.print_help()
        return

    ipList = None
    if options.ipListFile:
        ipList = ip_utils.IpList(open(args[0]), True);
    else:
        ipList = ip_utils.IpList(args, True)

    exclude = options.exclude == True;
    field = int(options.field) - 1 if options.field else None

    if field:
        for line in sys.stdin:
            fs = line.strip().split()
            try:
                ipInList = ipList.IpInList(fs[field]) is not None;

                if ipInList ^ exclude:
                    print line,;

            except:
                pass
    else:
        for line in sys.stdin:
            start = 0
            if options.mapreduce:
                start = line.find("\t") + 1

            i = start
            for c in line[start:]:
                if not c.isdigit() and c != ".":
                    break;
                i += 1;

            ipStr = line[start:i]
            ipInList = ipList.IpInList(ipStr) is not None;

            if ipInList ^ exclude:
                print line,;

if __name__ == "__main__":
    main();
