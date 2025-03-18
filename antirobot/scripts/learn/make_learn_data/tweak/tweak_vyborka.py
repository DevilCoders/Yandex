import os
import sys
from optparse import OptionParser

import yt.wrapper as yt

import devtools.fleur.util.yt as ytutil

from antirobot.scripts.learn.make_learn_data import setup_yt
from antirobot.scripts.learn.make_learn_data.tweak import rnd_request
from antirobot.scripts.learn.make_learn_data.tweak import tweak_task
from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import TweakList
from antirobot.scripts.learn.make_learn_data.tweak import pool


DEFAULT_MAX_WEIGHT = 0

E_NOT_ALL_DONE = 3

class Tweak:
    class TweakException(Exception):
        pass

    def __GetTaskOutDir(self, cls):
        return os.path.join(self.workDir, cls.NAME)

    def __init__(self, workDir, rndReqdataFile, **kw):
        self.workDir = workDir
        self.rndReqdataFile = rndReqdataFile
        print >>sys.stderr, "Start loading rndreq_data..."
        self.rndData = rnd_request.RndRequestData.Load(open(self.rndReqdataFile))
        print >>sys.stderr, "rndreq_data loaded"
        self.ytProxy = kw.get('ytProxy')
        self.ytToken = kw.get('ytToken')
        twList = kw.get('tweakList')
        self.tweakClasses = [i for i in TweakList.GetTweakClasses() if i.NAME in twList] if twList else TweakList.GetTweakClasses()
        self.multi = kw.get('multi', False)
        self.moretables = kw.get('moretables', False)
        self.maxWeight = kw.get('maxWeight', DEFAULT_MAX_WEIGHT) or DEFAULT_MAX_WEIGHT

    def IsRobot(self, featuresLine):
        return True

    def Prepare(self):
        def ForkedTask(cls):
            """ cls is a descendant class of TweakTask
                rndData is instace of RndRequestData (see rnd_request.py)
            """

            outDir = self.__GetTaskOutDir(cls)
            if not os.path.exists(outDir):
                os.mkdir(outDir)

            obj = cls(self.rndData, moretables=self.moretables)  # create instance of TweakTask descendant

            obj.UpdateRndReqFull(rnd_request.GetRndReqFullIter(self.rndReqdataFile))
            obj.Prepare(outDir)

        with ytutil.ModulesArchiveWriter() as writer:
            setup_yt.SetupYT(self.ytProxy, self.ytToken, writer)

            pol = pool.Scheduler(self.maxWeight)

            for cls in self.tweakClasses:
                if cls.NeedPrepare:
                    pol.Add(cls.Weight, ForkedTask, cls)

            pol.Run()
            if pol.GetFailedCount() > 0:
                raise Exception, 'Some tasks have failed'


    def LoadPrepared(self):
        self.checkers = []
        for cls in self.tweakClasses:
            instance = cls(self.rndData)
            instance.UpdateRndReqFull(rnd_request.GetRndReqFullIter(self.rndReqdataFile))
            if cls.NeedPrepare:
                instance.LoadPrepared(self.__GetTaskOutDir(cls))
            self.checkers.append(instance)

    def MakePreparedTweak(self, tweakCls):
        instance = tweakCls(self.rndData)
        instance.UpdateRndReqFull(rnd_request.GetRndReqFullIter(self.rndReqdataFile))
        if tweakCls.NeedPrepare:
            instance.LoadPrepared(self.__GetTaskOutDir(tweakCls))
        return instance

    def IsSuspicious(self, reqid):
        "Return class names returned true"
        posCheckers = []
        for checker in self.checkers:
            suspInfo = checker.SuspiciousInfo(reqid)
            if suspInfo:
                posCheckers.append(suspInfo)

        return posCheckers

    def PrintStat(self):
        for checker in self.checkers:
            checker.PrintStat()


def MakeTweaksSeparated(tweakInstance, resultDir, tweakList):
    print >>sys.stderr, "Checking for robot requests.."
    twClasses = TweakList.GetTweakClasses()
    if tweakList:
        twClasses = filter(lambda x: x.NAME in tweakList, twClasses)

    for twCls in twClasses:
        twInst = tweakInstance.MakePreparedTweak(twCls)

        outFile = open(resultDir + '/' + twCls.NAME + '.log', 'w')
        for rnd in tweakInstance.rndData:
            suspInfo = twInst.SuspiciousInfo(rnd.Raw.reqid)
            if suspInfo:
                print >>outFile, '\t'.join((rnd.Raw.reqid, str(suspInfo.coeff), suspInfo.name, suspInfo.descr))

        del twInst
