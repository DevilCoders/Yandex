#!/usr/bin/env python

import sys
import os

from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo

class Analyzer(TweakTask):
    NAME = 'hidden_image'

    NeedPrepare = False

#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self, **kw)
        self.rndRequests = rndRequestData

    def SuspiciousInfo(self, reqid):
        req = self.rndRequests.GetByReqid(reqid)
        if not req:
            return None

        if req.Flags.wasXmlSearch:
            return None

        if req.Flags.wasEnteredHiddenImage:
            return SuspInfo(coeff = 1, name = self.NAME, descr = '{}')

        return None


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)
