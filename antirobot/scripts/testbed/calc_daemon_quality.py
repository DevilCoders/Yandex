#!/usr/local/bin/python -OO

import sys;

def main():
    if len(sys.argv) < 3:
        print >>sys.stderr, "Usage: calc_daemon_quality.py <antirobot_daemon.log> <num robots>";
        sys.exit(1);

    daemonLogFileName = sys.argv[1];
    numRobots = int(sys.argv[2]);

    truePositives = 0;
    falsePositives = 0;
    numInvalid = 0;
    numTotal = 0;
    numUnsupported = 0;

    for line in open(daemonLogFileName):
        numTotal += 1;

        if not line.startswith("ENEMY"):
            continue;

        fields = line.rstrip().split("\t");

        if len(fields) < 13:
#            raise Exception("invalid line %s" % line);
            numInvalid += 1;
            continue;
        
        if fields[12] == "0" or fields[12] == "1": # old antirobot_daemons write access_log line as is, so robot marker goes after a tab
            robotFlag = fields[12] == "1";
        else:
#            raise Exception("unsupported line format: %s" % line);
            numUnsupported += 1;
            continue;

        if robotFlag:
            truePositives += 1;
        else:
            falsePositives += 1;

    falseNegatives = numRobots - truePositives;
    trueNegatives =  numTotal - numRobots - falsePositives;

    precision = float(truePositives) / max(truePositives + falsePositives, 1.0);
    recall = float(truePositives) / max(numRobots, 1.0);

    print "%s\ntrue pos: %d, false pos: %d, true neg: %d, false neg: %d, invalid %d, unsupported %d\nprecision: %0.2f%%, recall: %0.2f%%" % (
            daemonLogFileName,
            truePositives, falsePositives, trueNegatives, falseNegatives,  numInvalid, numUnsupported,
            precision * 100.0, recall * 100.0);

if __name__ == "__main__":
    main();

