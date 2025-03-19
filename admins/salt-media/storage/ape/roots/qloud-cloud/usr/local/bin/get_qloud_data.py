#!/usr/bin/env python
import sys
import json
q = "/etc/qloud/meta.json"
preset = {
    "dc": "datacenter",
    "cdm": "user_environment:COCAINE_DISCOVERY_MODIFICATION",
    "cr": "user_environment:COCAINE_RUNLIST"
}

if len(sys.argv) != 2:
    print("ERROR: no arg, exit, usage: get_qloud_data.py <data1>[,<data2>,<da:ta3>]" % str(e))
    sys.exit(10)

try:
    with open(q, "r") as f:
        qloud_data = json.loads(f.read())
    r = sys.argv[1].strip(",").split(",")
    nr = ""
    for rd in r:
        if rd in preset:
            nr = rd
            rd = preset[rd]
        if ":" not in rd:
            if nr == "dc":
                print(qloud_data[rd].lower())
            else:
                print(qloud_data[rd])
        else:
            tmpvar = qloud_data
            ds = rd.strip(":").split(":")
            for l in ds:
                if ds.index(l) == (len(ds) - 1):
                    print(tmpvar[l])
                else:
                    tmpvar = tmpvar[l]
    sys.exit(0)
except Exception as e:
    print("ERROR: %s" % str(e))
    sys.exit(1)

