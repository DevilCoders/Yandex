from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo


class Analyzer(TweakTask):
    NAME = 'suspicious'

    NeedPrepare = False

#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self, **kw)
        self.rndRequests = rndRequestData

    def SuspiciousInfo(self, reqid):
        req = self.rndRequests.GetByReqid(reqid)
        if not req:
            return None

        if req.suspicious and not req.ReqHasQuotes:
            return SuspInfo(coeff = 1, name = self.NAME, descr = 'susp request')

        return None


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)
