import sys

from antirobot.scripts.utils import ip_utils
from antirobot.scripts.learn.make_learn_data.tweak import susp_request

from antirobot.scripts.learn.make_learn_data.tweak import tweak_vyborka
from antirobot.scripts.learn.serp_images.extract_eventlog_data import RecordFieldIndices

TWEAK_WEIGHT_THREASHOLD = 100

def loadSpecialReqids(rndReqdataFileName):
    ajaxReqids = set();
    badTextReqids = set();
    for line in open(rndReqdataFileName):
        fields = line.strip().split('\t');
        if len(fields) < 8:
            raise Exception("invalid line:\n%s" % line);

        requestData = fields[3]
        if requestData.find("ajax=1") >= 0 or requestData.find("callback=jQuery") >= 0:
            ajaxReqids.add(fields[0]);

        if susp_request.IsSuspicious(fields):
            badTextReqids.add(fields[0]);

    return (ajaxReqids, badTextReqids);


def Execute(badSubnetsFile, featuresPure, rndCaptchasOneVersion, badSubnetsFile, rndReqData, resultTweakLog, tweakPreparedDir):
    badSubnets = ip_utils.IpList(open(badSubnetsFile), True) if badSubnetsFile else None
    (ajaxReqids, badTextReqids) = loadSpecialReqids(rndReqData) if rndReqData else (set(), set())

    if resultTweakLog:
        tweak = tweak_vyborka.Tweak(tweakPreparedDir, rndReqData)
        try:
            tweak.LoadPrepared()
        except Exception, e:
            print >>sys.stderr, "couldn't load prepared tweakers: " + str(e);
        tweakLog = open(resultTweakLog, 'w')

    resFile = open(featuresPure, 'w')

    for line in open(rndCaptchasOneVersion):
        fields = line.strip().split('\t');
        reqid = fields[RecordFieldIndices.REQID_INDEX];
        ip = fields[RecordFieldIndices.IP_INDEX];
        isRobot = fields[RecordFieldIndices.IMAGE_DOWNLOADED_INDEX] != '1';

        isBadSubnet = badSubnets is not None and badSubnets.IpInList(ip) is not None;
        isAjax = reqid in ajaxReqids;

        if resultTweakLog:
            tweakRules = []
            if isBadSubnet:
                tweakRules.append((TWEAK_WEIGHT_THREASHOLD, 'bad_subnet'))

            tweakRules += tweak.IsSuspicious(reqid)

            totalWeight = sum([x[0] for x in tweakRules])

            isRobotTweaked = totalWeight >= TWEAK_WEIGHT_THREASHOLD
            if len(tweakRules):
                print >>tweakLog, "%s\t%d\t%d\t%d\t%s" % (reqid, int(isRobot), int(isRobotTweaked), totalWeight, ';'.join([x[1] for x in tweakRules]))

            isRobot = isRobot or isRobotTweaked
        else:
            isRobot |= isBadSubnet;

        print >>resFile, "%s\t%d\t-\t-\t%s" % (reqid, int(isRobot), "\t".join(fields[RecordFieldIndices.FACTORS_START_INDEX:]));

    resFile.close()
