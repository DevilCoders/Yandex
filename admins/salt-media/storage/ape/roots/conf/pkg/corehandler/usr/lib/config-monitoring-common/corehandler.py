#!/usr/bin/env python

import sys
import json
import os
import os.path

chconfig = "/etc/corehandler.conf"
if len(sys.argv) > 1:
    if len(sys.argv[1]) > 1:
        chconfig = sys.argv[1]

graphitefile = "/usr/lib/yandex-graphite-checks/enabled/corehandler.py"
if len(sys.argv) > 2:
    if len(sys.argv[2]) > 1:
        graphitefile = sys.argv[2]
nameforsender = graphitefile.split("/")[-1]

iamsender = False
if sys.argv[0] == nameforsender or sys.argv[0] == graphitefile or \
    (len(sys.argv) >= 3 and sys.argv[2] == "graphite"):
    iamsender = True

if os.path.isfile(chconfig) and os.access(chconfig, os.R_OK):
    try:
        f = open(chconfig, 'r')
        conf = json.loads(f.read())
        f.close
    except:
        if iamsender:
            print("error 1")
            sys.exit(0)
        else:
            print("1;warn, corehandler config present but cant be read")
            sys.exit(0)
else:
    if iamsender:
        print("coredumps 0")
        sys.exit(0)
    else:
        print("0;ok")
        sys.exit(0)

try:
    monstate_enable = conf["monstate"]["enable"]
except:
    monstate_enable = 0

if monstate_enable == 0:
    if iamsender:
        print("coredumps 0")
        sys.exit(0)
    else:
        print("0;ok")
        sys.exit(0)

try:
    statepath = conf["monstate"]["file"]
except:
    if iamsender:
        print("error 2")
        sys.exit(0)
    else:
        print("1;warn, no monstate file in config")
        sys.exit(0)

tmpstatpath = "/tmp/coredumps.stat2graphite.tmp"


def monrunpart(statepath, tmpstatpath):
    if os.path.isfile(statepath) and os.access(statepath, os.R_OK):
        try:
            f = open(statepath, 'r')
            state = f.read().strip().split("\n")
            f.close
            os.remove(statepath)
        except Exception, e:
            print("2;crit, cant work with corehandler state file: %s" % str(e))
            sys.exit(0)
    else:
            print("0;ok")
            sys.exit(0)
    count_cores = {}
    last_dt = ""
    for line in state:
        if line.split(" ")[0] not in count_cores:
            count_cores[line.split(" ")[0]] = 1
            last_dt = line.split(" ")[1]
        else:
            count_cores[line.split(" ")[0]] += 1
            last_dt = line.split(" ")[1]
    answer = "2; coredump detected"
    total = 0
    for app, count in count_cores.iteritems():
        total += count
        answer += ", %s count %s times" % (app , count)
    try:
        f = os.open(tmpstatpath, os.O_RDWR | os.O_CREAT)
        try:
            count = int(os.read(f).strip())
        except:
            count = 0
        if count < 0:
            count = 0
        total = total + count
        os.write(f, str(total))
        os.close(f)
    except:
        pass
    print(answer)
    sys.exit(0)

def senderpart(tmpstatpath):
    if os.path.isfile(tmpstatpath) and os.access(tmpstatpath, os.R_OK):
        try:
            with open(tmpstatpath, 'r') as f:
                count = int(f.read().strip())
        except Exception as e:
                count = 0
        if count > 0:
            try:
                with open(tmpstatpath, 'w') as f:
                    f.write("0")
            except Exception as e:
                print("error 3")
                sys.exit(0)
    else:
            print("coredumps 0")
            sys.exit(0)
    print("coredumps %i" % count)
    sys.exit(0)


if iamsender:
    senderpart(tmpstatpath)
else:
    monrunpart(statepath, tmpstatpath)

