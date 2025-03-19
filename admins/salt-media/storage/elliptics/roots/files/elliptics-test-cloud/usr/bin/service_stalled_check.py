#!/usr/bin/python

import subprocess as s
import json

cmd = "mastermind couple list --state service-stalled --json"
p = s.Popen(cmd, stdout=s.PIPE, shell=True);
res = p.communicate()
broken = len(json.loads(res[0]))

if broken > 0:
    print "2;%d couples broken!"%broken
else:
    print "0;OK"
