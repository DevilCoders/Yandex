# -*- coding: utf-8 -*-

from devtools.fleur import util
from devtools.fleur.ytest import suite, test, generator, TestSuite
from devtools.fleur.ytest.tools.asserts import *

def GetCfgValue(config, key):
    for line in config.split('\n'):
        keyValPair = line.split('=')
        if len(keyValPair) == 2 and keyValPair[0].strip() == key:
            return keyValPair[1].strip()
    AssertTrue(False, message = "Key %s not found in cfg" % key)

@suite(package="antirobot.scripts")
class GenCfg(TestSuite):
    def CallGenCfgPy(self, argv = None, raiseOnError = True, timeout = None):
        argv = argv or []
        if '--geo' not in argv:
            argv += ['--geo', 'man']
        if '--active-instances' not in argv and '--current-configuration-id' not in argv:
            argv += ['--active-instances', 'production_antirobot_iss']

        genCfgPath = self.ResolvePathOfBinary('antirobot_gencfg')
        cmd = genCfgPath + ' ' + ' '.join(argv)
        return util.process.Execute(cmd, raiseOnError, timeout = timeout)

    def GenerateConfig(self, argv = None):
        return self.CallGenCfgPy(argv).GetStdOut()

    @generator(['-p', '--port'])
    def Port(self, portParam):
        port = '8888'
        cfg = self.GenerateConfig([portParam, port])
        AssertEqual(GetCfgValue(cfg, 'Port'), port)

    @test
    def DefaultProcessServerPort(self):
        port = 8888
        cfg = self.GenerateConfig(['-p', str(port)])
        AssertEqual(GetCfgValue(cfg, 'ProcessServerPort'), str(port + 1))

    @test
    def ProcessServerPort(self):
        port = '8888'
        cfg = self.GenerateConfig(['--process-port', port])
        AssertEqual(GetCfgValue(cfg, 'ProcessServerPort'), port)

    @generator(['man', 'sas', 'vla'])
    def GeoTag(self, geoTag):
        cfg = self.GenerateConfig(['--geo', geoTag])

        remoteWizardsFound = False
        for line in cfg.split('\n'):
            if line.strip().startswith('RemoteWizards'):
                value = line.strip()[len('RemoteWizards'):].strip()
                AssertNotEmpty(value)
                remoteWizardsFound = True
        AssertTrue(remoteWizardsFound)

    @generator(['-d', '--data'])
    def Data(self, dataParam):
        dataDir = 'custom_data'
        cfg = self.GenerateConfig([dataParam, dataDir])
        for param in ("BadUserAgentsFile", "BadUserAgentsNewFile", "GeodataBinPath", "LCookieKeysPath",
                      "PrivilegedIpsFile", "SpecialIpsFile", "TurboProxyIps",
                      "WhiteList", "YandexIpsFile"):
            AssertStartsWith(GetCfgValue(cfg, param), dataDir)

    @generator(['-f', '--formulas'])
    def Formulas(self, dataParam):
        formulasDir = 'custom_formulas'
        cfg = self.GenerateConfig([dataParam, formulasDir])
        AssertStartsWith(GetCfgValue(cfg, "MatrixnetFormulasDir"), formulasDir)

    @generator(['-r', '--runtime'])
    def RuntimeDataDir(self, rtParam):
        rtDir = 'custom_runtime'
        cfg = self.GenerateConfig([rtParam, rtDir])
        AssertEqual(GetCfgValue(cfg, 'RuntimeDataDir'), rtDir)

    @test
    def ReturnsNonZeroIfCmsTimesOut(self):
        # 0.001 секунды - слишком маленький таймаут для CMS, он в него точно не уложится
        # TODO: проверяется вообще не это. Сейчас нянин клиент бросает исключение при любом таймауте меньше 0.1
        launchResult = self.CallGenCfgPy(argv = ['--cms-timeout', '0.001', '--cms-tries', '1'],
                                         raiseOnError = False)
        AssertNotEqual(launchResult.GetReturnCode(), 0)

    @generator([10, 20, 50, 100])
    def NeverWorksTooLong(self, cmsTimeout):
        # бОльшую часть времени работы генератора конфига занимает обращение к CMS.
        # Происходит три обращения.
        # Поэтому время его работы должно быть не намного больше утроенного таймаута обращения к CMS.
        # В этом тесте нет ассертов, здесь проверяется, что скрипт всегда успевает
        # завершиться в течение (таймаута обращения к CMS)*6.
        self.CallGenCfgPy(argv = ['--cms-timeout', str(cmsTimeout), '--cms-tries', '1'],
                          raiseOnError = False, timeout = 6 * cmsTimeout)

    @generator([
        ["production_antirobot_iss"],
        ["production_antirobot_iss_prestable"],
        ["production_antirobot_iss", "production_antirobot_iss_prestable"],
    ])
    def AntirobotInstances(self, daemonsGroups):
        argv = []
        for daemonsGroup in daemonsGroups:
            argv += ["--active-instances", daemonsGroup]
        cfg = self.GenerateConfig(argv)

        port = GetCfgValue(cfg, "ProcessServerPort")
        allDaemons = GetCfgValue(cfg, "AllDaemons").split()
        AssertNotEmpty(allDaemons)

        for daemon in allDaemons:
            splitted = daemon.split(';')
            AssertEqual(len(splitted), 2)

            hostPort = splitted[0].rsplit(":", 1)
            ipPort = splitted[1].rsplit(":", 1)

            AssertEqual(len(hostPort), 2)
            AssertEqual(hostPort[1], port)
            AssertContains(hostPort[0], '.search.yandex.net')

            AssertEqual(len(ipPort), 2)
            AssertEqual(ipPort[1], port)
            AssertContains(ipPort[0], '[')
            AssertContains(ipPort[0], ']')
