# -*- coding: utf-8 -*-

from devtools.fleur.ytest import AppTestSuite, suite, test
from devtools.fleur.ytest.tools.asserts import *
from devtools.fleur import util

from antirobot.daemon.test.util import PortGenerator

import re
import sys

from urlparse import urljoin
from datetime import datetime
import tempfile

TASK_URL = "https://sandbox.yandex-team.ru/task/"

class NannyMock:
    def __init__(self, suite, port):
        self.Name = 'nanny_mock'
        self.Bin = 'nanny_mock'
        self.Suite = suite
        self.Port = port
        self.Host = "http://localhost:%d" % self.Port

    def Start(self):
        pathToSchema = util.path.GetSourcePath('antirobot', 'tools', 'nanny_mock', 'schema.json')
        params = ['--port', str(self.Port), '--schema', pathToSchema]
        self.Suite.StartApp(self.Name, self.Bin, params, self.Port)

    def Stop(self):
        self.Suite.StopApp(self.Name)


def GenerateSandboxTaskId():
    return datetime.now().strftime("%y%m%d%H%M%S%f")

@suite(package="antirobot.scripts.utils")
class NannyRelease(AppTestSuite):
    def SetupSuite(self):
        portGen = PortGenerator()

        self.NannyMock = NannyMock(self, portGen.next())
        self.NannyMock.Start()

    def TearDownSuite(self):
        self.NannyMock.Stop()

    def CallNannyRelease(self, sandboxTask, changelog=None, raiseOnError=True):
        changelog = changelog or 'Antirobot s1/r1\nrelease\n'
        with tempfile.NamedTemporaryFile() as cl:
            print >>cl, changelog
            cl.flush()

            yaPath = util.path.GetSourcePath('ya')
            path = self.GetSourcePath('antirobot', 'scripts', 'utils', 'nanny_release.py')
            cmd = ' '.join([yaPath, 'tool', 'python', path, '--verbose', '--nanny-host', self.NannyMock.Host,
                '--oauth-nanny', 'EMPTY_TOKEN', '--oauth-sandbox', 'EMPTY_TOKEN', '--ttl-to-inf', 'false', sandboxTask, cl.name])
            return util.process.Execute(cmd, raiseOnError=raiseOnError)


    @test
    def ReturnsReleaseRequestUrl(self):
        requestUrlPattern = urljoin(self.NannyMock.Host, r'/ui/#/r/(SANDBOX_RELEASE\-\d+)')

        ret = self.CallNannyRelease(sandboxTask=urljoin(TASK_URL, GenerateSandboxTaskId()))

        AssertMatches(requestUrlPattern, ret.GetStdOut())
        requestId = re.match(requestUrlPattern, ret.GetStdOut())
        AssertNotNone(requestId)


    @test
    def FailsIfGivenIncorrectSandboxUrl(self):
        ret = self.CallNannyRelease(sandboxTask='http://sandox.ru/12345', raiseOnError=False)
        AssertNotEqual(ret.GetReturnCode(), 0)

