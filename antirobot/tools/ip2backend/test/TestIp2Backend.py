# -*- coding: utf-8 -*-

from devtools.fleur import util
from devtools.fleur.ytest import suite, test, generator
from devtools.fleur.ytest.tools.asserts import *

from antirobot.daemon.test.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.test.util import GenRandomIP

from BaseHTTPServer import BaseHTTPRequestHandler
from StringIO import StringIO
import socket

DAEMON_COUNT = 4

# From http://stackoverflow.com/questions/2115410/does-python-have-a-module-for-parsing-http-requests-and-responses
class HttpRequest(BaseHTTPRequestHandler):
    def __init__(self, request_text):
        self.rfile = StringIO(request_text)
        self.raw_requestline = self.rfile.readline()
        self.error_code = self.error_message = None
        self.parse_request()

    def send_error(self, code, message):
        self.error_code = code
        self.error_message = message

def ParseOutput(data):
    lines = data.split('\n')
    AssertEqual(len(lines), 2)

    cachers = lines[0].split()
    AssertEqual(cachers[0], "Cachers:")

    processors = lines[1].split()
    AssertEqual(processors[0], "Processors:")
    return cachers[1:], processors[1:]


@suite(package="antirobot.tools")
class Ip2Backend(AntirobotTestSuite):
    def SetupSuite(self):
        """
        Этот тест сделан немного хачно. Мы запускаем только один Антиробот, но в AllDaemons
        указываем DAEMON_COUNT хостов. Дело в том, что Антиробот на запрос ip2backend'а возвращает
        имя своего хоста без порта. Так как в тестах все Антироботы запускаются на одной и той же
        машине, все они будут возвращать одно и то же имя хоста. Поэтому мы не можем в тестах
        сэмулировать наличие нескольких вертикалей.

        При этом мы можем протестировать, что ip2backend получает от Антиробота требуемое
        количество обрабатывающих машин. Именно поэтому мы в AllDaemons указываем DAEMON_COUNT
        адресов вместо одного.
        """

        super(Ip2Backend, self).SetupSuite()

        self.ThisHostName = socket.gethostname()
        thisHostIps = set([info[4][0] for info in socket.getaddrinfo(self.ThisHostName, 80)])
        yandexIpsFileName = self.GetWorkPath("yandex_ips")
        with open(yandexIpsFileName, "wt") as f:
            f.write("\n".join(thisHostIps))

        allDaemons = ["localhost:%d" % self.PortGen.next() for _ in range(DAEMON_COUNT)]
        daemon = self.StartDaemon({
            "YandexIpsFile" : yandexIpsFileName,
            "AllDaemons" : " ".join(allDaemons),
        })

        balancerBin = util.path.GetSourcePath('antirobot', 'tools', 'balancer_mock', 'balancer_mock.py')
        balancerPort = self.PortGen.next()
        args = ['-p', str(balancerPort), "--antirobot", "localhost:%d" % daemon.Port]
        self.StartApp("balancer_mock", balancerBin, args, balancerPort)

        self.Bin = self.GetBinPath("ip2backend")
        self.BalancerHost = "localhost:" + str(balancerPort)

    def RunIp2Backend(self, args=[], ip=None):
        return util.process.Execute([self.Bin, '--antirobot', self.BalancerHost, ip or '1.2.3.4'] + args,
            raiseOnError=True, shell=False)

    @generator(range(1, DAEMON_COUNT + 1))
    def Works(self, processorCount):
        result = self.RunIp2Backend(['--processors', str(processorCount)])

        AssertEqual(result.GetReturnCode(), 0)

        cachers, processors = ParseOutput(result.GetStdOut())
        AssertContains(cachers, self.ThisHostName)
        AssertEqual(len(processors), processorCount)

    @test
    def RequestMode(self):
        result = self.RunIp2Backend(['-p'])
        AssertEqual(result.GetReturnCode(), 0)

        request = HttpRequest(result.GetStdOut())
        AssertIsNone(request.error_code)

    @test
    def ReturnsDifferentProcessorsForDifferentIps(self):
        allProcessors = set()
        for ip in (GenRandomIP() for _ in xrange(15)):
            result = self.RunIp2Backend(ip=ip)
            AssertEqual(result.GetReturnCode(), 0)

            _, processors = ParseOutput(result.GetStdOut())
            allProcessors |= set(processors)

        AssertGreater(len(allProcessors), 1)
