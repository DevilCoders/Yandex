#!/skynet/python/bin/python

# # #!/usr/bin/env python2.6

import sys;
import math;
#from zlib import crc32;
from ctypes import c_ulonglong, c_uint;
from bisect import bisect_left;

sys.path.append('/home/asavin/work/arcadia/web/daemons/balancer/scripts/gencfg');
import utils;
import crc64;

## {{{ http://code.activestate.com/recipes/259177/ (r1)
# Initialisation
# 32 first bits of generator polynomial for CRC64
# the 32 lower bits are assumed to be zero

POLY64REVh = 0xd8000000L
CRCTableh = [0] * 256
CRCTablel = [0] * 256
isInitialized = False

def CRC64(aString):
    global isInitialized
    crcl = 0
    crch = 0
    if (isInitialized is not True):
        isInitialized = True
        for i in xrange(256): 
            partl = i
            parth = 0L
            for j in xrange(8):
                rflag = partl & 1L                
                partl >>= 1L               
                if (parth & 1):
                    partl |= (1L << 31L)
                parth >>= 1L
                if rflag:
                    parth ^= POLY64REVh
            CRCTableh[i] = parth;
            CRCTablel[i] = partl;

    print "%0X %0X" % (CRCTableh[1], CRCTablel[1]);
    for item in aString:
        shr = 0L
        shr = (crch & 0xFF) << 24
        temp1h = crch >> 8L
        temp1l = (crcl >> 8L) | shr                        
        tableindex = (crcl ^ ord(item)) & 0xFF
        
        crch = temp1h ^ CRCTableh[tableindex]
        crcl = temp1l ^ CRCTablel[tableindex]
    return (crch, crcl)

def CRC64digest(aString):
    return "%08X%08X" % (CRC64(aString))

## end of http://code.activestate.com/recipes/259177/ }}}

def genHosts():
    #hostGroups = '+NMETA2 +NMETA3 +NMETA4 +NMETA5 +NMETA6 +NMETA7 +NMETA8';
    hostGroups = '+NMETA2 +NMETA3 +NMETA5 +NMETA6 +NMETA7 +NMETA8';

    hosts = set()
    for group in hostGroups.split(' '):
        if group.startswith('-'):
            try:
                hosts -= set(_getHosts(group[1:]))
            except Exception, e:
                hosts.discard(group[1:])
        else:
            if group.startswith('+'):
                group = group[1:]
            try:
                hosts |= set(utils._getHosts(group))
            except Exception, e:
                hosts.add(group)
    return sorted(hosts);

def genWeights(hosts):
    weights = [];
    sum = 0.0
    for host in hosts:
        sum += utils._getMachineWeight(host);
        weights.append(sum);
    for i in range(len(weights)):
        weights[i] /= sum;
    return weights;

hosts = genHosts();
numHosts = len(hosts);
weights = genWeights(hosts);

def ip_to_num(ip):
    parts = ip.split(".")[:4];
    res = 0;
    for pp in parts:
        res = res * 256 + int(pp);
    return res;

def get_str_for_crc32(subnet):
    res = "";
    for i in range(2):
        res += chr(subnet & 0xFF);
        subnet = subnet >> 8;
    return res;

def subnet_hash(ipNum):
    ccc = crc64.crc64digest(get_str_for_crc32(ipNum >> 16));
    print ccc;
    return ccc;

def subnet_hash1(ipNum):
    ccc = CRC64digest(get_str_for_crc32(ipNum >> 16));
    res = c_ulonglong(int(ccc, 16));
    res1 = c_uint(int(ccc, 16));
    print ccc, res, res1;
    return res1;

def get_sfront_old(ipNum):
    return hosts[subnet_hash(ipNum) % numSfronts];

def get_sfront(ipNum):
    hash = subnet_hash(ipNum);
    return hosts[bisect_left(weights, hash / float(0xFFFFFFFFFFFFFFFF))];


if __name__ == "__main__":
    num = ip_to_num(sys.argv[1]);
    hash = subnet_hash(num);
    res = get_sfront(num);
    print numHosts, num >> 16, hash, res;

