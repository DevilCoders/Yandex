#!/usr/local/bin/python -OO

import sys;
import os;
import fnmatch;
import itertools;
from extract_eventlog_data import RecordFieldIndices;

def load_fnames_from_file(f):
    return filter(lambda x:x, [x.strip().strip('",') for x in f])

def load_fnames(dir):
    fnames = dict();
    for fn in os.listdir(dir):
        try:
            if fn.startswith("fnames_v"):
                ver = fn[8:];
            elif fn.startswith("factors_") and fn.endswith(".inc"):
                ver = fn[8:-4];
            else:
                continue;
        except:
            continue;

        with open("%s/%s" % (dir, fn)) as f:
            fnames[ver] = load_fnames_from_file(f);

        if len(set(fnames[ver])) < len(fnames[ver]):
            raise Exception("Duplicate factor names in %s" % fn);
        
        #print fn, ver, len(fnames[ver]);
    return fnames;

def make_factors_index(fnames):
    return dict(zip(fnames, itertools.count(0)));

def make_remaps(fnames, targetFactorsVer):
    targetFactorsIdx = make_factors_index(fnames[targetFactorsVer]);
    remaps = dict();
    for (ver, curFnames) in fnames.iteritems():
        remaps[ver] = tuple(targetFactorsIdx.get(x) for x in curFnames);
        #print ver, remaps[ver];
    return remaps;


def Execute(factorsVersionsDir, goodFactorsName, rndCaptchasOneVersion, rndCaptchas):
    fnames = load_fnames(factorsVersionsDir)

    if not goodFactorsName in fnames:
        raise Exception, "Target factors version invalid (no corresponding fnames file)";

    resFile = open(rndCaptchasOneVersion, 'w')
    remaps = make_remaps(fnames, goodFactorsName);

    targetNumFactors = len(fnames[goodFactorsName]);
    for line in open(rndCaptchas):
        fields = line.strip().split('\t');
        header = fields[:RecordFieldIndices.FACTORS_START_INDEX];
        ver = fields[RecordFieldIndices.FACTORS_VERSION_INDEX];
        if ver == "0":
            ver = "1";
        factors = fields[RecordFieldIndices.FACTORS_START_INDEX:];

        # thischeck is for special version of factors in 's17-r11-learning_by_serp_picture' branch with serp images
        # very ugly, but shit happens
        if ver == '11' and len(factors) == 3936:
            ver = '11.1'

        # shit has happened again
        if ver == '12' and len(factors) == 3952:
            ver = '12.1'

        if len(factors) != len(fnames[ver]):
            raise Exception("invalid factors len: %d (must be %d), header: %s" % (len(factors), len(fnames[ver]),
                "\t".join(fields[:RecordFieldIndices.FACTORS_START_INDEX])));

        newFactors = ["0"] * targetNumFactors;
        remap = remaps[ver];
        for (f, r) in zip (factors, remap):
            if r != None:
                newFactors[r] = f;

        print >>resFile, '\t'.join(header + newFactors)
