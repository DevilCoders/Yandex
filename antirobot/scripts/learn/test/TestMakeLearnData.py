# -*- coding: utf-8 -*-

import subprocess
import sys
import copy
import time
import datetime
import logging
import json

from devtools.fleur import util
from devtools.fleur.ytest import suite, test, TestSuite, timeout, generator
from devtools.fleur.ytest.tools.asserts import *
from devtools.fleur.util.yt_launcher import DownloadYT, YtLauncher
from antirobot.daemon.test.AntirobotTestSuite import ANTIROBOT_TEST_DATA
from antirobot.scripts.learn.make_learn_data.tweak import tweak_list

from yt import wrapper as yt


def GetLineCount(fileName):
    res = 0
    for line in open(fileName):
        res += 1

    return res


def LoadFeaturesData(line):
    fs = line.split('\t')
    return json.loads(fs[2])


@suite(package="antirobot.scripts.learn")
class MakeLearnData(TestSuite):
    DATE = datetime.date(2018, 8, 1)

    def SetupSuite(self):
        super(MakeLearnData, self).SetupSuite()
        self.MapreduceBin = os.path.join(self.GetTestData(owner=ANTIROBOT_TEST_DATA, type='input'), 'mapreduce')
        self.TestDataPath = os.path.join(self.GetTestData(owner='antirobot.scripts.learn.test_data', type='input'), 'data')

        self.MakeLearnDataBin = self.GetBinPath('make_learn_data')
        workPath = self.GetWorkPath()
        self.YtRootPath = self.GetWorkPath('YT_LOCAL')
        DownloadYT(self.YtRootPath)

        self.YtLauncher = YtLauncher(self.YtRootPath, workPath)
        if not self.YtLauncher.Start():
            raise Exception("Could not start local YT")

        yt.config['proxy']['url'] = self.YtLauncher.YtProxy
        self.PrepareYt()

    def TearDownSuite(self):
        self.YtLauncher.Stop()

    def PrepareYt(self):
        StrDate = lambda d: d.strftime("%Y-%m-%d")
        strDate = StrDate(self.DATE)

        yt.create("table", "//logs/antirobot-binary-event-log/1d/%s" % strDate, recursive=True)
        yt.create("table", "//logs/reqans-log/1d/%s" % strDate, recursive=True)
        yt.create("table", "//logs/redir-log/1d/%s" % strDate, recursive=True)
        yt.create("table", "//user_sessions/preprod/pub/search/daily/%s/frauds" % strDate, recursive=True)
        yt.create("table", "//logs/yandex-access-log/1d/%s" % strDate, recursive=True)
        yt.create("table", "//logs/yandex-access-log/1d/%s" % StrDate(self.DATE - datetime.timedelta(days=1)), recursive=True)
        yt.create("table", "//logs/yandex-access-log/1d/%s" % StrDate(self.DATE + datetime.timedelta(days=1)), recursive=True)

        yt.create("map_node", "//home/antirobot/tmp", recursive=True)

        dayTable = '//logs/antirobot-binary-event-log/1d/%s' % self.DATE.strftime('%Y-%m-%d')
        yt.write_table(dayTable, open(self.TestDataPath), format="json", raw=True)


    def MakeLearnData(self, workDir, zone='all', service=None, useTweaks=False):
        tweakNames = tweak_list.TweakList.GetTweakNames()
        tweakLevelsFile = self.GetWorkPath('tweak-levels')
        with open(tweakLevelsFile, 'w') as f:
            for tw in tweakNames:
                print >>f, '%s=10' % tw

        fLog = open(self.GetWorkPath('workDir', 'make_learn_data.error.log'), 'w')
        if not fLog:
            raise Exception, "Bad log"

        options = [
            '--server', self.YtLauncher.YtProxy,
            '--scheme', 'random_captchas',
            '--work-dir', workDir,
            '--zone', zone,
            '--hack-list', 'all',
            '--clear',
            '--factors-versions-path', self.GetSourcePath('antirobot', 'daemon_lib', 'factors_versions')
            ]

        if useTweaks:
            options.extend([
            '--with-tweaks',
            '--include-tweaks', ','.join(tweakNames),
            '--tweak-levels', tweakLevelsFile,
            ])

        if service:
            options.extend(['--service', service])

        DateStr = lambda x: x.strftime('%Y%m%d')
        cmd = [self.MakeLearnDataBin] + options + ['make', DateStr(self.DATE), DateStr(self.DATE)]
        env = copy.copy(os.environ)
        #env['PYTHONPATH'] = ':'.join([
            #self.GetSourcePath('devtools', 'fleur', 'imports', 'arcadia'),
            #os.path.join(self.YtRootPath, 'python')
        #    ])
        util.process.Execute(cmd, env=env, raiseOnError=True, shell=False, stderr=None)

    @test
    @timeout(240)
    def SchemeRandomCaptchas(self):
        workDir = util.path.MakeDir(self.GetWorkPath('workDir'), removeExisting=True)
        self.MakeLearnData(workDir, useTweaks=True)

        with util.path.ChangeDir(workDir):
            AssertEqual(GetLineCount('features_pure.txt'), 40)
            AssertEqual(GetLineCount('features_tweaked'), 40)
            AssertEqual(GetLineCount('rnd_captchas_one_version'), 40)
            AssertEqual(GetLineCount('rnd_reqdata'), 40)

            canonResult = {
                # COM
                '1533070977397815-13791086277767969276': 1, # web .COM
                '1533071188024497-6670104834724631239': 1, # web
                '1533071620608126-13210879079179980333': 0, # web
                # CUBR
                '1533070894703362-14843455507246612760': 1, # autoru
                '1533071275791012-9293886630770956852': 1, # webmaster
                '1533087000793420-5810756839895002921': 0, # tech
                '1533088346766960-13649130310301282552': 0, # web
                '1533089340564923-1223728053346035942': 1, # taxi
                '1533090378622179-10768707313742352607': 1, # web
                '1533091199200149-14520132041829239541': 0, # web
                '1533103721874228-5853787120095557891': 0, # web
                '1533104327925110-2449866320417533761': 1, # realty
                '1533106673368700-5965237746579213574': 1, # realty
                '1533108287125293-9900506935054804095': 1, # web
                '1533108880772793-17353216241539285489': 1, # market
                '1533109827056419-11443222832335824751': 0, # kinopoisk
                '1533110196559643-6760502883711697263': 1, # autoru
                '1533110483682683-16763080410857000402': 0, # avia
                '1533113402061441-10838582225387009331': 1, # taxi
                '1533113740028150-4083088407198813369': 1, # web
                '1533116906217787-1793849596670206963': 0, # autoru
                '1533117501638284-3341686311592303627': 1, # autoru
                '1533119890699481-7897626906675157010': 1, # autoru
                '1533120191839231-8800748805403512669': 1, # autoru
                '1533120215743964-11853303691544889717': 1, # wordstat
                '1533125037585906-5372977334070194531': 1, # webmaster
                '1533125225315248-9410362817539574683': 1, # avia
                '1533127174073990-4750628431878684184': 0, # rabota
                '1533130440777337-3065597052885409213': 1, # avia
                '1533130767764677-5835988839164625461': 0, # web
                '1533136170823687-17389220295811649363': 1, # web
                '1533138446694820-17728263373253056424': 1, # market
                '1533141402427677-14027816811147830378': 0, # web
                '1533142921246956-2837682180444931428': 0, # realty
                '1533143546412873-15642301050616604072': 0, # kinopoisk
                '1533144639654120-15190330807653626648': 1, # market
                '1533145901290446-16373851537269882164': 0, # avia
                '1533147566421123-8630458328711990376': 0, # rabota
                '1533150117495351-6736170886327667540': 0, # kinopoisk
                '1533156695984399-1931884267682180700': 0, # rabota
                }

            for x in open('features_pure.txt'):
                fs = x.split('\t')
                feat = LoadFeaturesData(x)
                AssertEqual(int(fs[1]), canonResult[feat['req_id']])

    @generator([
        ('com', {
                '1533070977397815-13791086277767969276': 1, # web .COM
                '1533071188024497-6670104834724631239': 1, # web
                '1533071620608126-13210879079179980333': 0, # web
            }),
        ( 'cubr',{
                '1533070894703362-14843455507246612760': 1, # autoru
                '1533071275791012-9293886630770956852': 1, # webmaster
                '1533087000793420-5810756839895002921': 0, # tech
                '1533088346766960-13649130310301282552': 0, # web
                '1533089340564923-1223728053346035942': 1, # taxi
                '1533090378622179-10768707313742352607': 1, # web
                '1533091199200149-14520132041829239541': 0, # web
                '1533103721874228-5853787120095557891': 0, # web
                '1533104327925110-2449866320417533761': 1, # realty
                '1533106673368700-5965237746579213574': 1, # realty
                '1533108287125293-9900506935054804095': 1, # web
                '1533108880772793-17353216241539285489': 1, # market
                '1533109827056419-11443222832335824751': 0, # kinopoisk
                '1533110196559643-6760502883711697263': 1, # autoru
                '1533110483682683-16763080410857000402': 0, # avia
                '1533113402061441-10838582225387009331': 1, # taxi
                '1533113740028150-4083088407198813369': 1, # web
                '1533116906217787-1793849596670206963': 0, # autoru
                '1533117501638284-3341686311592303627': 1, # autoru
                '1533119890699481-7897626906675157010': 1, # autoru
                '1533120191839231-8800748805403512669': 1, # autoru
                '1533120215743964-11853303691544889717': 1, # wordstat
                '1533125037585906-5372977334070194531': 1, # webmaster
                '1533125225315248-9410362817539574683': 1, # avia
                '1533127174073990-4750628431878684184': 0, # rabota
                '1533130440777337-3065597052885409213': 1, # avia
                '1533130767764677-5835988839164625461': 0, # web
                '1533136170823687-17389220295811649363': 1, # web
                '1533138446694820-17728263373253056424': 1, # market
                '1533141402427677-14027816811147830378': 0, # web
                '1533142921246956-2837682180444931428': 0, # realty
                '1533143546412873-15642301050616604072': 0, # kinopoisk
                '1533144639654120-15190330807653626648': 1, # market
                '1533145901290446-16373851537269882164': 0, # avia
                '1533147566421123-8630458328711990376': 0, # rabota
                '1533150117495351-6736170886327667540': 0, # kinopoisk
                '1533156695984399-1931884267682180700': 0, # rabota
            }),
        ])
    @timeout(240)
    def SchemeRandomCaptchasByZone(self, zone, canon):
        workDir = util.path.MakeDir(self.GetWorkPath('workDir'), removeExisting=True)
        self.MakeLearnData(workDir, zone=zone)

        canonLen = len(canon)
        with util.path.ChangeDir(workDir):
            AssertEqual(GetLineCount('features_pure.txt'), canonLen)
            AssertEqual(GetLineCount('features_tweaked'), canonLen)
            AssertEqual(GetLineCount('rnd_captchas_one_version'), canonLen)
            AssertEqual(GetLineCount('rnd_reqdata'), canonLen)

            for x in open('features_pure.txt'):
                fs = x.split('\t')
                feat = LoadFeaturesData(x)
                AssertEqual(int(fs[1]), canon[feat['req_id']])

    @generator([
        ('kinopoisk', {
             '1533109827056419-11443222832335824751': 0, # kinopoisk
             '1533143546412873-15642301050616604072': 0, # kinopoisk
             '1533150117495351-6736170886327667540': 0, # kinopoisk
            }),

        ('realty', {
             '1533104327925110-2449866320417533761': 1, # realty
             '1533106673368700-5965237746579213574': 1, # realty
             '1533142921246956-2837682180444931428': 0, # realty
            }),
        ('autoru', {
             '1533070894703362-14843455507246612760': 1, # autoru
             '1533110196559643-6760502883711697263': 1, # autoru
             '1533116906217787-1793849596670206963': 0, # autoru
             '1533117501638284-3341686311592303627': 1, # autoru
             '1533119890699481-7897626906675157010': 1, # autoru
             '1533120191839231-8800748805403512669': 1, # autoru
            }),
        ('market', {
             '1533108880772793-17353216241539285489': 1, # market
             '1533138446694820-17728263373253056424': 1, # market
             '1533144639654120-15190330807653626648': 1, # market
            }),
        ('avia', {
                '1533110483682683-16763080410857000402': 0, # avia
                '1533125225315248-9410362817539574683': 1, # avia
                '1533130440777337-3065597052885409213': 1, # avia
                '1533145901290446-16373851537269882164': 0, # avia
            }),
        ('web', {
                '1533070977397815-13791086277767969276': 1, # web .COM
                '1533071188024497-6670104834724631239': 1, # web
                '1533071620608126-13210879079179980333': 0, # web
                '1533088346766960-13649130310301282552': 0, # web
                '1533090378622179-10768707313742352607': 1, # web
                '1533091199200149-14520132041829239541': 0, # web
                '1533103721874228-5853787120095557891': 0, # web
                '1533108287125293-9900506935054804095': 1, # web
                '1533113740028150-4083088407198813369': 1, # web
                '1533130767764677-5835988839164625461': 0, # web
                '1533136170823687-17389220295811649363': 1, # web
                '1533141402427677-14027816811147830378': 0, # web
            }),
        ])
    @timeout(240)
    def SchemeRandomCaptchasByService(self, service, canon):
        workDir = util.path.MakeDir(self.GetWorkPath('workDir'), removeExisting=True)
        self.MakeLearnData(workDir, service=service)

        canonLen = len(canon)
        with util.path.ChangeDir(workDir):
            AssertEqual(GetLineCount('features_pure.txt'), canonLen)
            AssertEqual(GetLineCount('features_tweaked'), canonLen)
            AssertEqual(GetLineCount('rnd_captchas_one_version'), canonLen)
            AssertEqual(GetLineCount('rnd_reqdata'), canonLen)

            for x in open('features_pure.txt'):
                fs = x.strip().split('\t')
                fs = x.split('\t')
                feat = LoadFeaturesData(x)
                AssertEqual(int(fs[1]), canon[feat['req_id']])
