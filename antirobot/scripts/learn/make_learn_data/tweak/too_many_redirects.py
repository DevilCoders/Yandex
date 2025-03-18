import sys

from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo


class Analyzer(TweakTask):
    NAME = 'too_many_redirects'

    NeedPrepare = False

#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self, **kw)
        self.ajaxReqids = set()
        self.suspTextReqids = set()
        self.rndRequests = rndRequestData

    def UpdateRndReqFull(self, rndReqFullIter):
        for req in rndReqFullIter:
            if req.isAjax:
                self.ajaxReqids.add(req.Raw.reqid)
            if req.suspicious:
                self.suspTextReqids.add(req.Raw.reqid)

    def SuspiciousInfo(self, reqid):
        req = self.rndRequests.GetByReqid(reqid)
        if not req:
            print >>sys.stderr, req, ' not found'
            return None

        numRemovals = max(int(req.NumRedir.numRemovals), 1)
        numRedirects = int(req.NumRedir.numRedirs)

        if reqid in self.ajaxReqids:
            return None

        if ((reqid in self.suspTextReqids) and numRedirects >= (10 * numRemovals)) or (numRedirects >= (50 * numRemovals)):
            return SuspInfo(coeff = 2, name = self.NAME, descr = '%s,%s' % (req.NumRedir.numRedirs, req.NumRedir.numRemovals))

        return None


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)
