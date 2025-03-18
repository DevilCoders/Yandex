import sys
import os
import fnmatch
import itertools

DEBUG=os.environ.get('DEBUG', False)

from antirobot.scripts.learn.make_learn_data.data_types import FinalCaptchaData


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

        print >>sys.stderr, fn, ver, len(fnames[ver]);

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

    lineNum = 0
    for line in open(rndCaptchas):
        lineNum += 1
        captchaData = FinalCaptchaData.FromString(line.strip())
        ver = captchaData.Factors.Version()

        # thischeck is for special version of factors in 's17-r11-learning_by_serp_picture' branch with serp images
        # very ugly, but shit happens
        if captchaData.Factors.version == '11' and len(captchaData.Factors.factors) == 3936:
            ver = '11.1'

        # shit has happened again
        if captchaData.Factors.version == '12' and len(captchaData.Factors.factors) == 3952:
            ver = '12.1'

        if len(captchaData.Factors.factors) != len(fnames[ver]):
            error = "line %d: invalid factors len: %d (must be %d for ver %s) %s" % (lineNum, len(captchaData.Factors.factors), len(fnames[ver]), captchaData.Factors.version)
            if DEBUG:
                print >>sys.stderr, error
                continue
            raise Exception(error)

        newData = captchaData.Copy()
        newData.Factors.version = '-'
        newData.Factors.factors = ["0"] * targetNumFactors;

        remap = remaps.get(ver)
        if not remap:
            raise Exception("Unknown version of factors ('%s'). Please use latest <factors-versions> directory" % ver)

        for (f, r) in zip (captchaData.Factors.factors, remap):
            if r != None:
                newData.Factors.factors[r] = f

        print >>resFile, newData.ToString()

    resFile.close()
