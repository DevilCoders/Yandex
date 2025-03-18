import os

try:
    from library.python import resource
except:
    resource = None

from antirobot.scripts.utils import ip_utils
from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo


BAD_SUBNETS_NAME = '/bad_subnets'


def ReadBadsubnetsFile(filePath):
    if filePath:
        return open(patternsPath).readlines()
    elif resource:
        return resource.find(BAD_SUBNETS_NAME).split('\n')
    else:
        raise Exception("Could not load bad_subnets file")


class Analyzer(TweakTask):
    NAME = 'bad_subnets'

    NeedPrepare = False

#public:
    def __init__(self, rndRequestData, fileName=None, **kw):
        TweakTask.__init__(self, **kw)
        self.rndRequests = rndRequestData
        self.badSubnets = ip_utils.IpList(ReadBadsubnetsFile(fileName), True)

    def SuspiciousInfo(self, reqid):
        req = self.rndRequests.GetByReqid(reqid)
        if not req:
            return None

        ipRange = self.badSubnets.IpInList(req.Raw.ip)
        if ipRange is not None:
            return SuspInfo(coeff = 3, name = self.NAME, descr = '%s in %s' % (req.Raw.ip, str(ipRange)))

        return None


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)
