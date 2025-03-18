#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import socket
import sys
_curdir = os.path.dirname(__file__)
sys.path.append(
    os.path.abspath(os.path.join(
        _curdir, '..', '..', '..', '..', 'devtools', 'fleur')))

from imports.arcadiahook import ArcadiaImportHook
sys.meta_path.append(ArcadiaImportHook(['yweb', 'library'], buildType='debug'))

import library.messagebus.bindings.python.pymessagebus as pymessagebus
import yweb.crawlrank.scripts.orangembusproto.pyorangembusproto as pyorangembusproto
import yweb.crawlrank.protos.data_pb2 as protos

mb = pymessagebus.TMessageBus()
control_protocol = pyorangembusproto.ControlProtocol()
mb.Register(control_protocol, socket.getfqdn(), 10500)

session = mb.CreateSyncSource(control_protocol)

for i in xrange(10000):
    req = protos.TControlRequest()
    req.Command = "foo-{0}".format(i)
    res = session.Send(req)
    print res
