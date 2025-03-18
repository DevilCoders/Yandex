# coding: utf-8

import sys
import os
import fnmatch
import itertools
import re

factor_base_re = re.compile('[^|^]*')
factor_base = lambda f: factor_base_re.match(f).group()

forbidden_factors = set((
    'req_other',
    'host_other',
    'rep_other',
    'qlang_foreign',
    'qlang_other',
    'geo_city',
))

def good_factor(factor):
    fs = factor.strip().strip('",')
    return factor_base(fs) not in forbidden_factors

def load_last_fnames(dir):
    lastFnames = []
    lastVer = 0

    for fn in os.listdir(dir):
        try:
            if fn.startswith("fnames_v"):
                ver = float(fn[8:])
            elif fn.startswith("factors_") and fn.endswith(".inc"):
                ver = int(fn[8:-4])
            else:
                continue
        except:
            continue

        if ver > lastVer:
            lastVer = ver

            with open("%s/%s" % (dir, fn)) as f:
                lastFnames = list(f)

    print >>sys.stderr, "Used version: %s" % lastVer
    return lastFnames


def Execute(factorsVersionsDir, goodFactorsFile):
    goodFnames = filter(good_factor, load_last_fnames(factorsVersionsDir))
    open(goodFactorsFile, 'w').write("".join(goodFnames))
