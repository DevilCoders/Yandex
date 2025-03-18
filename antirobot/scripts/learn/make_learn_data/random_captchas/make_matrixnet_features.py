import sys
import os.path
import getopt
import re

import os
import imp
import json

from antirobot.scripts.utils import ip_utils
from antirobot.scripts.learn.make_learn_data.tweak import susp_request

from antirobot.scripts.learn.make_learn_data import data_types


def ParseReqdata(lines):
    for line in lines:
        parsed = json.loads(line)
        flags = parsed["Flags"]
        del flags["py/object"]
        host_search = re.search(r"Host: ([\w.]+)\W", parsed["Raw"]["request"])
        host = host_search.group(1) if host_search else None
        yield {
            "request": parsed["Raw"]["request"],
            "service": parsed["Raw"]["service"],
            "location": parsed["Raw"]["request"].split(" ")[1].split("?")[0],
            "host": host,
            "ident_type": parsed["Raw"]["uidStr"],
            "flags": flags,
            "ip": parsed["Raw"]["ip"],
            "req_id": parsed["Raw"]["reqid"],
        }
    return


def Execute(featuresPure, rndCaptchasOneVersion, rndReqData):
    resFile = open(featuresPure, 'w')

    parsedReqData = {item["req_id"]: json.dumps(item) for item in ParseReqdata(open(rndReqData))}
    for line in open(rndCaptchasOneVersion):
        data = data_types.FinalCaptchaData.FromString(line.strip())
        isRobot = not data.Flags.wasImageShow
        print >>resFile, "%s\t%d\t%s\t1\t%s" % (data.Ident.reqid[:10],
                                                int(isRobot),
                                                parsedReqData[data.Ident.reqid],
                                                "\t".join([str(x) for x in data.Factors.factors]))

    resFile.close()
