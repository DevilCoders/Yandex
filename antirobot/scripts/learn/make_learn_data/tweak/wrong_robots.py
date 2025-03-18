import sys
from optparse import OptionParser

from antirobot.scripts.learn.make_learn_data.tweak import rnd_request


ACTION_LIST = (
    ('m_video', 'clear m.vidio.yandex.ru'),
    ('x_wap_profile', 'clear x-wap-profile'),
    )


def Do_m_video(req, reqidSet):
    host = req.headers.get('Host')
    if host and host == 'm.video.yandex.ru':
        reqidSet.add(req.Raw.reqid)
#        print '%s\tm_video' % req.reqid


def Do_x_wap_profile(req, reqidSet):
    if req.headers.get('x-wap-profile'):
        reqidSet.add(req.Raw.reqid)
#        print 'x_wap\t%s' % req.reqid


def FillReqidsToUnset(rndreqFile, useActions, reqids):
    print >>sys.stderr, useActions
    for req in rnd_request.GetRndReqFullIter(rndreqFile):
        if 'm_video' in useActions:
            Do_m_video(req, reqids)

        if 'x_wap_profile' in useActions:
            Do_x_wap_profile(req, reqids)


def PrintReqids(resultFileName, featuresFile, reqids):
    resFile = open(resultFileName, 'w')
    for i in open(featuresFile):
        fs = i.split('\t')
        reqid = fs[0]
        isRobot = fs[1] == '1'
        if isRobot and reqid in reqids:
            print >>resFile, reqid

def Execute(rndReqData, hackList, untweakLog, featuresPure):
    reqids = set()

    FillReqidsToUnset(rndReqData, hackList, reqids)
    PrintReqids(untweakLog, featuresPure, reqids)
