#!/usr/bin/env python
# coding: utf-8

import os
import sys
import imp
import logging
import argparse
import datetime
import shutil
import pickle

import arcadia
import yt.wrapper as yt

import library.python.resource as resource
from devtools.fleur.util import path

from antirobot.scripts.learn.make_learn_data import tweak_factor_names
from antirobot.scripts.learn.make_learn_data import split_vyborka

from antirobot.scripts.learn.make_learn_data.tweak import apply_tweaks
from antirobot.scripts.learn.make_learn_data.tweak import tweak_vyborka
from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import TweakList
from antirobot.scripts.learn.make_learn_data.tweak import wrong_robots

from antirobot.scripts.learn.make_learn_data.random_captchas import extract_eventlog_data as r_extract_eventlog_data
from antirobot.scripts.learn.make_learn_data.random_captchas import add_tweaking_factors as r_add_tweaking_factors
from antirobot.scripts.learn.make_learn_data.random_captchas import convert_factors_to_one_version as r_convert_factors_to_one_version
from antirobot.scripts.learn.make_learn_data.random_captchas import make_matrixnet_features as r_make_matrixnet_features


SCHEME_RANDOM_CAPTCHAS = 'random_captchas'
SCHEME_SERP_IMAGES = 'serp_images'

DEFAULT_SPLIT_THRESHOLD = 0.9
OPTS_FILE_NAME = 'OPTS'

STAGES = (
    STAGE_INITIAL,
    STAGE_CAPTCHAS_EXTRACTED,
    STAGE_FACTOR_NAMES_TWEAKED,
    STAGE_PURE_FEATURES_MAKED,
    STAGE_TWEAK_PREPARED,
    STAGE_TWEAK_SEPARATED,
    STAGE_LEARNSETS_PREPARED,
    ) = range(7)


def ParseArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument('--help-commands', action='store_true', help='print commands help')
    parser.add_argument('-s', '--server', action='store', dest='ytProxy', help='yt server')
    parser.add_argument('--yt-token', action='store', dest='ytToken', help='set yt token')
    parser.add_argument('--scheme', action='store', dest='schemeName', default=SCHEME_RANDOM_CAPTCHAS, help='learn scheme to use (default random captcha')
    parser.add_argument('--work-dir', action='store', default='.', dest='workDir', help='a dir where to save intermediate files')
    parser.add_argument('--zone', action='store', dest='zone', default='all', help='zone to get features for')
    parser.add_argument('--clear', action='store_true', dest='clear', help='clear already prepared data (start process from begining)')
    parser.add_argument('--with-tweaks', action='store_true', dest='withTweaks', help='enable tweaks')
    parser.add_argument('--include-tweaks', action='store', dest='tweakList', help='list of tweaks separated by comma')
    parser.add_argument('--par-weight', dest='paralWeight', action='store', type=int,  help='maximum summ of task weights running simulataneously')
    parser.add_argument('--tweak-levels', dest='tweakLevels', action='store', help="path to file with redefined tweak levels")
    parser.add_argument('--tweak-threshold', dest='tweakThreshold', action='store', type=float, help="threshold when check suspicious (default 100)")
    parser.add_argument('--hack-list', dest='hackList', action='store', help="use hacks separated by comma (default mvideo,x_wap_profile)")
    parser.add_argument('--split-threshold', dest='splitThreshold', action='store', help="threshold to split learn set (defult 0.5)")
    parser.add_argument('--services', dest='services', action='store', help="get features only for requests to given services (separated by comma)")
    parser.add_argument('--factors-versions-path', dest='factorsVerPath', action='store', required=True)

    parser.add_argument('cmd', help='command (make, continue, tweak_list, status, extract_test_data, load_test_data)', nargs='?')
    parser.add_argument('dateBegin', help='start date', nargs='?')
    parser.add_argument('dateEnd', help='end date', nargs='?')

    return parser.parse_args()


def PrintHelpCommands():
    print '''Available commands:
    make                    make learn features
    tweak_list              print available tweaks
    status                  print status
    extract_test_data       extract events to put to tests
    '''

def StrToDate(strDate):
    return datetime.date(int(strDate[0:4]), int(strDate[4:6]), int(strDate[6:8]))


def MakeDateRange(d1, d2):
    from datetime import date, timedelta;
    assert(type(d1) == date and type(d2) == date)

    res = [];
    d = d1;
    while d <= d2:
        res.append(d.strftime("%Y-%m-%d"));
        d += timedelta(days = 1);

    return res


class Scheme:
    def __init__(self, schemeName):
        if schemeName == SCHEME_RANDOM_CAPTCHAS:
            self.ExtractEventlogData = lambda ctx: r_extract_eventlog_data.Execute(
                ctx.YtProxy,
                ctx.YtToken,
                ctx.Zone,
                ctx.ServiceList,
                ctx.DatesRange,
                ctx.RndReqDataRaw,
                ctx.RndCaptchas
                )

            self.ExtractTestData = lambda ctx: r_extract_eventlog_data.ExtractTestData(
                ctx.YtProxy,
                ctx.YtToken,
                ctx.DatesRange
                )

            self.AddTweakingFactors = lambda ctx: r_add_tweaking_factors.Execute(
                ctx.GoodFactorsFile,
                ctx.RndReqDataRaw,
                ctx.RndCaptchasOneVersion,
                ctx.RndReqData
                )

            self.ConvertFactorsToOneVersion = lambda ctx: r_convert_factors_to_one_version.Execute(
                ctx.FactorsVersionsDir,
                ctx.GoodFactorsName,
                ctx.RndCaptchasOneVersion,
                ctx.RndCaptchas
                )

            self.MakeMatrixnetFeatures = lambda ctx: r_make_matrixnet_features.Execute(
                ctx.FeaturesPure,
                ctx.RndCaptchasOneVersion,
                ctx.RndReqData
                )

        elif schemeName == SCHEME_SERP_IMAGES:
            raise Exception("Unsupported learn scheme")
            #from antirobot.scripts.learn.serp_images import extract_eventlog_data
            #from antirobot.scripts.learn.serp_images import add_tweaking_factors
            #from antirobot.scripts.learn.serp_images import make_matrixnet_features
            #from antirobot.scripts.learn.serp_images import convert_factors_to_one_version

            #self.ExtractEventlogData = lambda ctx: extract_eventlog_data.Execute(
            #    ctx.MrServer,
            #    ctx.MrUser,
            #    ctx.MrProxy,
            #    ctx.Zone,
            #    ctx.DatesRange,
            #    ctx.MrExec,
            #    ctx.RndReqDataRaw,
            #    ctx.RndCaptchas
            #    )

            #self.ExtractTestData = lambda ctx: None

            #self.AddTweakingFactors = lambda ctx: add_tweaking_factors.Execute(
            #    ctx.GoodFactorsFile,
            #    ctx.RndReqDataRaw,
            #    ctx.RndReqData
            #    )

            #self.ConvertFactorsToOneVersion = lambda ctx: convert_factors_to_one_version.Execute(
            #    ctx.FactorsVersionsDir,
            #    ctx.GoodFactorsName,
            #    ctx.RndCaptchasOneVersion,
            #    ctx.RndCaptchas
            #    )

            #self.MakeMatrixnetFeatures = lambda ctx: make_matrixnet_features.Execute(
            #    ctx.BadSubnetsFile,
            #    ctx.FeaturesPure,
            #    ctx.RndCaptchasOneVersion,
            #    ctx.BadSubnetsFile,
            #    ctx.RndReqData,
            #    ctx.ResultTweakLog,
            #    ctx.TweakPreparedDir
            #    )

        else:
            raise Exception, "Unknown learn scheme '%s'" % schemeName



class Context:
    @staticmethod
    def TouchFile(fileName):
        open(fileName, 'a+').close()

    @staticmethod
    def TouchDir(dirName):
        if not os.path.exists(dirName):
            os.mkdir(dirName)

    def __init__(self, opts):
        self.Scheme = Scheme(opts.schemeName)
        self.Command = opts.cmd
        self.WorkDir = opts.workDir
        self.SavedOptsFile = os.path.join(opts.workDir, OPTS_FILE_NAME)
        self.BeginDate = StrToDate(opts.dateBegin) if opts.dateBegin else datetime.date.today()
        self.EndDate = StrToDate(opts.dateEnd) if opts.dateEnd else self.BeginDate
        self.DatesRange = MakeDateRange(self.BeginDate, self.EndDate)
        self.YtToken = opts.ytToken
        self.YtProxy = opts.ytProxy
        self.Zone = opts.zone
        self.ServiceList = opts.services.split(',') if opts.services else []
        self.SrcFactorsVersionsDir = opts.factorsVerPath
        self.FactorsVersionsDir = os.path.join(self.WorkDir, 'factors_versions')
        self.WithTweaks = opts.withTweaks
        self.TweakList = opts.tweakList.split(',') if opts.tweakList else TweakList.GetTweakNames()
        self.ParallelTaskWeight = opts.paralWeight
        self.TweakLevelsFile = opts.tweakLevels
        self.TweakThreshold = opts.tweakThreshold
        self.HackList = opts.hackList.split(',') if opts.hackList else []
        self.SplitThreshold = opts.splitThreshold or DEFAULT_SPLIT_THRESHOLD

        self.RndReqDataRaw = os.path.join(self.WorkDir, 'rnd_reqdata_raw')
        self.RndReqData = os.path.join(self.WorkDir, 'rnd_reqdata')
        self.RndCaptchas = os.path.join(self.WorkDir, 'rnd_captchas')
        self.RndCaptchasOneVersion = os.path.join(self.WorkDir, 'rnd_captchas_one_version')
        self.GoodFactorsName = 'good'
        self.GoodFactorsFile = os.path.join(self.FactorsVersionsDir, 'factors_%s.inc' % self.GoodFactorsName)
        self.FeaturesPure = os.path.join(self.WorkDir, 'features_pure.txt')
        self.BadSubnetsFile = path.GetSourcePath('antirobot', 'scripts', 'learn', 'bad_subnets')
        self.TweakPreparedDir = os.path.join(self.WorkDir, 'tweak_prepared')
        self.TweakSeparatedDir = os.path.join(self.WorkDir, 'tweak_separated')
        self.FeaturesTweaked = os.path.join(self.WorkDir, 'features_tweaked')
        self.FeaturesTweakedLearn = os.path.join(self.WorkDir, 'features_tweaked.learn')
        self.FeaturesTweakedTest = os.path.join(self.WorkDir, 'features_tweaked.test')
        self.FeaturesHacked = '%s.hacked' % self.FeaturesPure
        self.ParalWeight = opts.paralWeight
        self.ResultTweakLog = os.path.join(self.WorkDir, 'tweak_log')
        self.UntweakLog = os.path.join(self.WorkDir, 'untweak_log')

    def PrepareWorkDir(self, opts):
        if opts.clear:
            names = os.listdir(self.WorkDir)
            for name in names:
                fname = os.path.join(self.WorkDir, name)
                if os.path.isdir(fname):
                    shutil.rmtree(fname)
                else:
                    os.remove(fname)
        elif os.path.exists(opts.workDir) and os.path.exists(self.SavedOptsFile):
            savedOpts = pickle.load(open(self.SavedOptsFile))
            if savedOpts != opts:
                raise Exception, "Supplied options are not equal with the intially used"
        else:
            pickle.dump(opts, open(self.SavedOptsFile, 'w'))


        self.TouchDir(self.FactorsVersionsDir)
        self.TouchDir(self.TweakPreparedDir)
        self.TouchDir(self.TweakSeparatedDir)

        self.TouchFile(self.RndReqDataRaw)
        self.TouchFile(self.RndReqData)
        self.TouchFile(self.RndCaptchas)
        self.TouchFile(self.RndCaptchasOneVersion)
        self.TouchFile(self.GoodFactorsFile)
        self.TouchFile(self.FeaturesPure)
        self.TouchFile(self.FeaturesTweaked)
        self.TouchFile(self.FeaturesTweakedLearn)
        self.TouchFile(self.FeaturesTweakedTest)
        self.TouchFile(self.FeaturesHacked)


def GetSavedOpts(workDir):
    return pickle.load(open(os.path.join(workDir, OPTS_FILE_NAME)))


class Manager:
    RND_DATA = 'rnd_data'
    STAGE_FILE_NAME = 'STAGE'

    def __init__(self, context):
        self.Context = context
        self.StageFileName = os.path.join(self.Context.WorkDir, self.STAGE_FILE_NAME)
        self.Prepare()
        self.Stage = self.ReadStage()

    def Prepare(self):
        for l in os.listdir(self.Context.SrcFactorsVersionsDir):
            shutil.copy(os.path.join(self.Context.SrcFactorsVersionsDir, l),
                        self.Context.FactorsVersionsDir)


        if not os.path.exists(self.StageFileName):
            self.WriteStage(STAGE_INITIAL)

    def ReadStage(self):
        try:
            return int(open(self.StageFileName).read())
        except:
            return STAGE_INITIAL

    def WriteStage(self, stage):
        open(self.StageFileName, 'w').write(str(stage))

    def MakeRndData(self):
        if self.ReadStage() >= STAGE_CAPTCHAS_EXTRACTED:
            return

        logging.info('Making rnd_data...')

        self.Context.Scheme.ExtractEventlogData(self.Context)

        self.WriteStage(STAGE_CAPTCHAS_EXTRACTED)

    def TweakFactorsNames(self):
        if self.ReadStage() >= STAGE_FACTOR_NAMES_TWEAKED:
            return

        logging.info('Tweaking factors names')
        tweak_factor_names.Execute(self.Context.FactorsVersionsDir, self.Context.GoodFactorsFile)

        self.Context.Scheme.ConvertFactorsToOneVersion(self.Context)
        self.Context.Scheme.AddTweakingFactors(self.Context)

        self.WriteStage(STAGE_FACTOR_NAMES_TWEAKED)

    def MakePureFeatures(self):
        if self.ReadStage() >= STAGE_PURE_FEATURES_MAKED:
            return

        self.Context.Scheme.MakeMatrixnetFeatures(self.Context)

        self.WriteStage(STAGE_PURE_FEATURES_MAKED)

    def PrepareTweaks(self):
        if self.ReadStage() >= STAGE_TWEAK_PREPARED:
            return

        if self.Context.WithTweaks:
            tweak = tweak_vyborka.Tweak(
                self.Context.TweakPreparedDir,
                self.Context.RndReqData,
                ytProxy=self.Context.YtProxy,
                ytToken=self.Context.YtToken,
                tweakList=self.Context.TweakList,
                maxWeight=self.Context.ParallelTaskWeight
                )
            tweak.Prepare()
        else:
            shutil.copy(self.Context.FeaturesPure, self.Context.FeaturesTweaked)

        self.WriteStage(STAGE_TWEAK_PREPARED)

    def MakeSeparatedTweaks(self):
        if self.ReadStage() >= STAGE_TWEAK_SEPARATED:
            return

        if self.Context.WithTweaks:
            tweak = tweak_vyborka.Tweak(
                self.Context.TweakPreparedDir,
                self.Context.RndReqData,
                ytProxy=self.Context.YtProxy,
                ytToken=self.Context.YtToken,
                tweakList=self.Context.TweakList,
                maxWeight=self.Context.ParallelTaskWeight
                )
            tweak_vyborka.MakeTweaksSeparated(tweak, self.Context.TweakSeparatedDir, self.Context.TweakList)

        self.WriteStage(STAGE_TWEAK_SEPARATED)

    def PrepareLearnSets(self):
        if self.ReadStage() >= STAGE_LEARNSETS_PREPARED:
            return

        def ApplyHacks():
            if self.Context.HackList:
                wrong_robots.Execute(
                    self.Context.RndReqData,
                    self.Context.HackList,
                    self.Context.UntweakLog,
                    self.Context.FeaturesPure
                    )
                apply_tweaks.Unapply(
                    self.Context.UntweakLog,
                    self.Context.FeaturesHacked,
                    self.Context.FeaturesTweaked
                    )
            else:
                open(self.Context.UntweakLog, 'w') # create empty file
                shutil.copy(self.Context.FeaturesTweaked, self.Context.FeaturesHacked)

        if self.Context.WithTweaks:
            apply_tweaks.Apply(
                self.Context.ResultTweakLog,
                self.Context.TweakSeparatedDir,
                self.Context.TweakList,
                self.Context.TweakLevelsFile,
                self.Context.FeaturesPure,
                self.Context.FeaturesTweaked,
                self.Context.TweakThreshold
                )

        ApplyHacks()

        split_vyborka.Execute(
            self.Context.SplitThreshold,
            self.Context.FeaturesHacked,
            self.Context.FeaturesTweakedLearn,
            self.Context.FeaturesTweakedTest
            )

        self.WriteStage(STAGE_LEARNSETS_PREPARED)


def Make(mgr):
    mgr.MakeRndData()
    mgr.TweakFactorsNames()
    mgr.MakePureFeatures()

    mgr.PrepareTweaks()
    mgr.MakeSeparatedTweaks()

    mgr.PrepareLearnSets()

    print "All done"


def ListTweaks():
    for name in TweakList.GetTweakNames():
        print name


def main():
    opts = ParseArgs()

    if opts.help_commands:
        PrintHelpCommands()
        sys.exit(1)

    if opts.cmd in ('make', 'continue'):
        if opts.cmd == 'continue':
            opts = GetSavedOpts(opts.workDir)
        context = Context(opts)
        context.PrepareWorkDir(opts)
        mgr = Manager(context)
        Make(mgr)
    elif opts.cmd == 'list_tweaks':
        ListTweaks()
    elif opts.cmd == 'extract_test_data':
        context = Context(opts)
        context.Scheme.ExtractTestData(context)
    else:
        print >>sys.stderr, "Unknown command"
        sys.exit(2)


if __name__ == "__main__":
    main()
