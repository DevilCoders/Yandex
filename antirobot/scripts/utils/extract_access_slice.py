#!/usr/bin/env python

from mapreducelib import MapReduce, Record
import zlib;
from ip_utils import *;

def isGoodIp(ip):
    if len(ip) <= 2 or ip == "0.0.0.0":
        return False;

    subnetMod1000 = (ipToNum(ip) >> 16) % 1000;
    return subnetMod1000 == 123;

class MapperExtract:
    def __init__(self, timeBeg, timeEnd):
        self.timeBeg = timeBeg;
        self.timeEnd = timeEnd;

    def __call__(self, rec):
        try:
            fields = rec.value.split(' ');

            if not isGoodIp(fields[0]):
                return;

            timeStr = fields[-7];
            t = float(timeStr);
            if t < self.timeBeg or t >= self.timeEnd:
                return;
        except:
            return;

        yield Record(timeStr, rec.value);

def main():
    MapReduce.useDefaults(verbose = True, usingSubkey = False, server="abies00:8013", saveSource = True);

    mapper = MapperExtract(1274414402, 1274417002);
    MapReduce.runMap(mapper, srcTable = "access_log/20100521", dstTable = "tmp/asavin/ac01", sortMode = True);


if __name__ == "__main__":
    main();

