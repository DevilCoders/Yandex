# -*- coding: utf-8 -*-

from devtools.fleur import util
from devtools.fleur.ytest import suite, TestSuite, generator, test
from devtools.fleur.ytest import AssertEqual, AssertNotEqual, AssertEmpty

from antirobot.scripts.support import genaccessip
from antirobot.scripts.support.genaccessip import TURBO_IPS, SPECIAL_IPS, YANDEX_IPS, WHITELIST_IPS, PRIVILEGED_IPS

import os
import urllib
import re

TIMEOUT = 180


@suite(package="antirobot.scripts.support")
class GenIpLists(TestSuite):
    @generator([
            ('::1', True),
            ('ff::1', True),
            ('ABcd:0:34:fF', True),
            ('ABcd::0:34:fF', True),
            ('1234:123:0::/64', True),
            ('1234:123:FFFF/64', True),
            ('1::/4', True),
            ('0G:11/4', False),
            ('AB:12:32:56:78:90:01:12/1', True),
            ('AB:12:32:56:78:90:01:12/127', True),
            ('AB:12:32:56:78:90:01:12/128', False),
            ('AB:12:32:56:78:90:01:12:/1', False),
            ('AB:12:32:56:78:90:01:12:11', False),
        ])
    def IsValidIp6Addr(self, inputVal, expect):
        AssertEqual(genaccessip.IsValidIp6Addr(inputVal), expect)


    @generator([
            ('1.2.3.4', True),
            ('1.2.3.4/123', True),
            ('1.2.3.4/ 123', False),
            ('000.00.0.255', True),
            ('a.b.c.d', False),
            ('1.2.3', False),
            ('1.2.3..4', False),
            ])
    def IsIp4(self, inputVal, expect):
        AssertEqual(genaccessip.IsIp4(inputVal), expect)


    @generator([
            ('abc.def', True),
            ('abc.def.ghi', True),
            ('a1.b2.c', True),
            ('a1.b2.3', False),
            ('abcd', False),
            ])
    def IsHost(self, inputVal, expect):
        AssertEqual(genaccessip.IsHost(inputVal), expect)

    @generator([
            ('_macro_', True),
            ('_macro', False),
            ('macro_', False),
            ('macro', False),
            ('macro.com', False),
            ('_1_', False),
            ('_a1_', True),
            ])
    def IsMacro(self, inputVal, expect):
        AssertEqual(genaccessip.IsMacro(inputVal), expect)


@suite(package="antirobot.scripts.support")
class GenaccessipScript(TestSuite):
    def SetupSuite(self):
        self.sourceDir = self.GetWorkPath('source')
        self.scriptPath = os.path.join(self.GetTestPath(), '../genaccessip.py')

    def SetupTest(self):
        util.path.MakeDir(self.sourceDir, removeExisting=True)

    def RunScript(self, sourceName):
        cmd = ' '.join([self.scriptPath, '--no-check-ipv4', '--src-dir', self.sourceDir, '--out-dir', self.sourceDir,
                        '--src-file', sourceName, '--out-file', self.GetDstName(sourceName)])
        return util.process.Execute(cmd, False, timeout=TIMEOUT).GetReturnCode()

    def GetSrcPath(self, name):
        return os.path.join(self.sourceDir, name)


    def GetDstName(self, name):
        parts = name.split('.')
        withNoExt = name
        if len(parts) > 1:
            withNoExt = '.'.join(parts[0:-1])

        return withNoExt + '.new'


    def GetDstPath(self, name):
        return os.path.join(self.sourceDir, self.GetDstName(name))


    def MakeSource(self, name, lines):
        with open(self.GetSrcPath(name), 'w') as h:
            for l in lines:
                print >>h, l


    def MakeEmptySources(self):
        self.MakeSource(TURBO_IPS, [])
        self.MakeSource(SPECIAL_IPS, [])
        self.MakeSource(YANDEX_IPS, [])
        self.MakeSource(WHITELIST_IPS, [])
        self.MakeSource(PRIVILEGED_IPS, [])


    @generator([TURBO_IPS, SPECIAL_IPS, YANDEX_IPS, WHITELIST_IPS])
    def MissingSource(self, sourceName):
        self.MakeEmptySources()
        util.path.RemovePath(self.GetSrcPath(sourceName))
        AssertNotEqual(self.RunScript(sourceName), 0)


    @generator([
        TURBO_IPS,
        SPECIAL_IPS,
        YANDEX_IPS]
    )
    def RightOutput(self, sourceName):
        self.MakeEmptySources()
        self.MakeSource(sourceName, ['1.2.3.4'])
        AssertEqual(self.RunScript(sourceName), 0)
        AssertNotEqual(os.path.getsize(self.GetDstPath(sourceName)), 0)


    @test
    def MustBeUnchanged(self):
        self.MakeEmptySources()
        lines = ['1.2.3.4', '::1', '1234:5678:ff:FF::/64']
        self.MakeSource(YANDEX_IPS, lines)

        AssertEqual(self.RunScript(YANDEX_IPS), 0)
        translated = [x.strip() for x in open(self.GetDstPath(YANDEX_IPS)).readlines()]
        AssertEqual(translated, lines)


    @generator([
        'sandbox.yandex.ru',
        '_TANKNETS_',
        'https://racktables.yandex.net/export/networklist.php?report=usernets'
        ])
    def Translated(self, singleLine):
        self.MakeEmptySources()
        lines = [singleLine]
        self.MakeSource(YANDEX_IPS, lines)

        AssertEqual(self.RunScript(YANDEX_IPS), 0)
        translated = [x.strip() for x in open(self.GetDstPath(YANDEX_IPS)).readlines()]

        AssertNotEqual(translated, lines)

        lineWithoutSharpPresent = False
        for l in lines:
            if not l.startswith('#'):
                lineWithoutSharpPresent = True
                break

        AssertEqual(lineWithoutSharpPresent, True)


    @generator([TURBO_IPS, SPECIAL_IPS, YANDEX_IPS, WHITELIST_IPS, PRIVILEGED_IPS])
    def AllMacrosExist(self, listName):
        MACROS_URL = 'http://ro.admin.yandex-team.ru/data/macros-inc.m4'
        NON_MACRO_CHARACTER_REGEXP = '[^A-Za-z\\d_]'

        macroDefinitions = urllib.urlopen(MACROS_URL).read()

        unknownMacros = []

        for line in open(os.path.join(self.GetTestPath(), "..", listName)):
            line = line.strip()
            if line.startswith('_'):
                macro = line.split('#', 1)[0].strip()
                regexp = re.compile(NON_MACRO_CHARACTER_REGEXP + macro + NON_MACRO_CHARACTER_REGEXP)
                if not regexp.search(macroDefinitions):
                    unknownMacros.append(macro)

        AssertEmpty(unknownMacros)
