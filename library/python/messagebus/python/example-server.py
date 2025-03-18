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

control_server = mb.CreateSyncServer(control_protocol)
for msg in control_server.IncomingMessages():
    req = protos.TControlRequest()
    req.ParseFromString(msg.GetPayload())
    res = protos.TControlResponse()
    res.Status = req.Command + " done"
    control_server.Reply(msg, res)
