# -*- coding: utf-8 -*-

# Running (in the current file directory):
#
# $ /path/to/arcadia/yweb/fleur/ci/ytest/ytest.py test \
#       --build-dir /path/to/build/dir \
#       --work-dir /tmp/dir \
#       --build-type <build-type>
#
# for example,
# $ ../../../../../yweb/fleur/ci/ytest/ytest.py test --work-dir ~/tmp --display-errors backtrace

import signal
import socket
import multiprocessing

from devtools.fleur.ytest import TestSuite, suite, test, constraint
from devtools.fleur.ytest.tools import AssertEqual, AssertContains, AssertNotNone
from devtools.fleur.util import systemdefaults

# Note that some sensitive imports are placed at local (function/method) level to avoid
# conflict when suite is being discovered. During discovery framework imports source
# code and everything global is being executed. This might cause some dynamic conflicting
# libraries to load.
def CreateTestProtocol():
    import library.messagebus.bindings.python.test.testproto.pytestproto as testproto
    return testproto.TestProtocol()

def CreateMessageBus():
    import library.messagebus.bindings.python.test.testproto.pytestproto as testproto
    return testproto.TMessageBus()

def RunTest(serverFunc, clientFunc):
    def server_func_wrap():
        # even if shit happens and parent process dies
        # current process will receive SIGALRM signal and exit
        signal.alarm(2)
        serverFunc()
    server = multiprocessing.Process(target=server_func_wrap)
    server.start()
    try:
        clientFunc()
    finally:
        server.terminate()
        server.join()
    AssertContains([0, -15], server.exitcode)

@suite(package="MessageBus")
@constraint('library.messagebus.bindings')
class Bindings(TestSuite):

    @test
    def Sanity(self):
        import library.messagebus.bindings.python.pymessagebus as pymessagebus
        AssertNotNone(pymessagebus.TMessageBus())

    @test
    def CustomProtocol(self):
        from library.messagebus.bindings.python.test.testproto.testmessage_pb2 import TTestMessage

        def serverFunc():
            mb = CreateMessageBus()
            protocol = CreateTestProtocol()
            mb.Register(protocol, socket.getfqdn(), systemdefaults.GetBasePortForUserTest())
            controlServer = mb.CreateSyncServer(protocol)
            for msg in controlServer.IncomingMessages():
                req = TTestMessage()
                req.ParseFromString(msg.GetPayload())
                res = TTestMessage()
                res.Payload = "reply to " + req.Payload
                reqs, ress = str(req), str(res)
                controlServer.Reply(msg, res)

        def clientFunc():
            mb = CreateMessageBus()
            protocol = CreateTestProtocol()
            mb.Register(protocol, socket.getfqdn(), systemdefaults.GetBasePortForUserTest())
            session = mb.CreateSyncSource(protocol)
            for i in xrange(10):
                req = TTestMessage()
                msg = "foo-{0}".format(i)
                req.Payload = msg
                res = session.Send(req)
                AssertEqual(res.Payload, "reply to " + msg)

        RunTest(serverFunc, clientFunc)

    @test
    def ServerDropMessage(self):
        from library.messagebus.bindings.python.test.testproto.testmessage_pb2 import TTestMessage

        def serverFunc():
            mb = CreateMessageBus()
            protocol = CreateTestProtocol()
            mb.Register(protocol, socket.getfqdn(), systemdefaults.GetBasePortForUserTest())
            controlServer = mb.CreateSyncServer(protocol)
            for msg in controlServer.IncomingMessages():
                break

        def clientFunc():
            mb = CreateMessageBus()
            protocol = CreateTestProtocol()
            mb.Register(protocol, socket.getfqdn(), systemdefaults.GetBasePortForUserTest())
            session = mb.CreateSyncSource(protocol, False)
            for i in xrange(1):
                req = TTestMessage()
                msg = "foo-{0}".format(i)
                req.Payload = msg
                session.Send(req, needReply=False)

        RunTest(serverFunc, clientFunc)
