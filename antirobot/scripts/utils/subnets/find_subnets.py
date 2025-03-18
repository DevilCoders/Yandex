#!/usr/local/bin/python
# input must be sorted by ip (as strings)

import sys;
import math;

def ip_to_num(ip):
    parts = ip.split(".");
    res = 0;
    for pp in parts:
        res = res * 256 + int(pp);
    return res;

def find80PercentOfReqs(reqs):
    reqs.sort();
    reqs.reverse();
    totalReqs = sum(reqs);
    nr = 0;
    for i in range(len(reqs)):
        nr += reqs[i];
        if nr >= 0.8 * totalReqs:
            return i + 1;
    return None;        

def main():
    short = len(sys.argv) > 1 and sys.argv[1] == "-s";

    STRICT = False;

    prevMask = "";
    minIp = "0";
    maxIp = "0";
    numRobotQueries = 0;
    numTotalQueries = 0;
    minNT = 10000000;
    maxNT = 0;
    numIps = 0;
    minNR = 0;
    maxNR = 0;

    robotReqs = [];

    for line in sys.stdin:
        hasSlash = False;

        nr = 0;
        nt = 0;

        if short:
            ip = line.strip();
        else:
            (ip, queries) = line.strip().split("\t");
            nums = [int(x) for x in queries.split("/")];
            nr = nums[0];
            if len(nums) > 1:
                nt = nums[1];
        
        mask = ".".join(ip.split(".")[:3]);

        if mask != prevMask:
            if minIp != maxIp and (not STRICT or ip_to_num(maxIp) - ip_to_num(minIp) <= numIps * 5):
                if short:
                    print "%s - %s\t%d" % (minIp, maxIp, numIps);
                elif nt > 0:
                    print "%s - %s\t%d/%d\t%d\t%d\t%d" % (minIp, maxIp, numRobotQueries, numTotalQueries, numIps, minNT, maxNT);
                else:
                    print "%s - %s\t%d\t%d\t%d\t%d\t%d" % (minIp, maxIp, numRobotQueries, numIps, minNR, maxNR, find80PercentOfReqs(robotReqs));

            minIp = "0";
            maxIp = "0";
            numRobotQueries = 0;
            numTotalQueries = 0;
            minNT = 1000000000;
            maxNT = 0;
            minNR = 1000000000;
            maxNR = 0;
            numIps = 0;
            del robotReqs[:];

        numRobotQueries += nr;
        numTotalQueries += nt;
        numIps += 1;

        minNT = min(minNT, nt);
        maxNT = max(maxNT, nt);
        minNR = min(minNR, nr);
        maxNR = max(maxNR, nr);
         
        if minIp == "0":
            minIp = ip;
            maxIp = ip;
        else:
            if ip_to_num(minIp) > ip_to_num(ip):
                minIp = ip;
            if ip_to_num(maxIp) < ip_to_num(ip):
                maxIp = ip;

        robotReqs.append(nr);
        prevMask = mask;


    if minIp != maxIp and (not STRICT or ip_to_num(maxIp) - ip_to_num(minIp) <= numIps * 5):
        if short:
            print "%s - %s\t%d" % (minIp, maxIp, numIps);
        elif nt > 0:
            print "%s - %s\t%d/%d\t%d\t%d\t%d" % (minIp, maxIp, numRobotQueries, numTotalQueries, numIps, minNT, maxNT);
        else:
            print "%s - %s\t%d\t%d\t%d\t%d\t%d" % (minIp, maxIp, numRobotQueries, numIps, minNR, maxNR, find80PercentOfReqs(robotReqs));

if __name__ == "__main__":
    main();

