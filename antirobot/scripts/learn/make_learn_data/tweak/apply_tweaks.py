import sys
import os
from optparse import OptionParser
from collections import defaultdict
import re

from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo
from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import TweakList


DEFAULT_THRESHOLD = 30
DEFAULT_TWEAK_LEVEL = 10

class TweakData:
    def __init__(self):
        self.data = defaultdict(lambda: list()) # reqid => list of SuspInfo
        self.tweakLevels = {}

        for n in TweakList.GetTweakNames():
            self.tweakLevels[n] = DEFAULT_TWEAK_LEVEL

    def UpdateTweakLevels(self, fileName):
        for i in open(fileName):
            (name, value) = i.strip().split('=')
            name = name.strip()
            if name in self.tweakLevels:
                self.tweakLevels[name] = float(value.strip())

    def AppendFrom(self, fileName):
        f = open(fileName)

        for i in f:
            (reqid, coeff, name, descr) = i.strip().split('\t')
            susp = SuspInfo(coeff, name, descr)
            self.data[reqid].append(susp)

    def IsSuspicious(self, suspList, threshold=DEFAULT_THRESHOLD):
        if suspList:
            totalLevel = self.TotalLevel(suspList)
            if totalLevel >= threshold:
                return True

        return False

    def GetSuspList(self, reqid):
        return self.data[reqid]

    def TotalLevel(self, suspList):
        if not suspList:
            return 0

        res = 0
        for s in suspList:
            level = self.tweakLevels.get(s.name)
            if level == None:
                print >>sys.stderr, 'Bad tweak name: %s' ^ s.name
                continue
            res += level * float(s.coeff)

        return res


reTweak = re.compile(r'([^/]+)\.log')


def Apply(resultTweakLog, tweakSeparatedDir, tweakList, tweakLevelsFile, featuresPure, featuresTweaked, tweakThreshold):
    def ReadTweakPathsToAppend(srcDir):
        res = []
        for d in os.listdir(srcDir):
            m = reTweak.search(d)
            if m:
                res.append((m.group(1), d))

        return res

    tweakLog = open(resultTweakLog, 'w')
    tweakFiles = ReadTweakPathsToAppend(tweakSeparatedDir)

    tweakFiles = filter(lambda x: x[0] in tweakList, tweakFiles)  # filter by include

    tw = TweakData()
    if tweakLevelsFile:
        tw.UpdateTweakLevels(tweakLevelsFile)

    for d in tweakFiles:
        print >>sys.stderr, "Add %s..." % d[1],
        tw.AppendFrom('%s/%s' % (tweakSeparatedDir, d[1]))
        print >>sys.stderr, "done"

    resFile = open(featuresTweaked, 'w')
    threshold = tweakThreshold or DEFAULT_THRESHOLD
    for i in open(featuresPure):
        i = i.strip()
        fs = i.split('\t')

        reqid = fs[0]
        isRobot = int(fs[1])

        featStr = i
        if not isRobot:
            suspList = tw.GetSuspList(reqid)
            if suspList and tw.IsSuspicious(suspList, threshold):
                print >>tweakLog, '\t'.join([fs[0], ','.join([x.name for x in suspList]), str(tw.TotalLevel(suspList))])
                fs[1] = '1'
                featStr = '\t'.join(fs)

        print >>resFile, featStr


def Unapply(untweakLog, featuresHacked, featuresTweaked):
    untweakFile = open(untweakLog)

    tweakReqids = set()
    for i in untweakFile:
        tweakReqids.add(i.strip().split('\t')[0].strip())

    print >>sys.stderr, "%d records will be marked non robot" % len(tweakReqids)

    resFile = open(featuresHacked, 'w')
    for i in open(featuresTweaked):
        i = i.strip()
        fs = i.split('\t')

        reqid = fs[0].strip()
        isRobot = int(fs[1])

        featStr = i
        if isRobot:
            if reqid in tweakReqids:
                fs[1] = '0'
                featStr = '\t'.join(fs)

        print >>resFile, featStr
