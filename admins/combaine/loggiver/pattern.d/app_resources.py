#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Collect various resources about cocaine combainer's apps
"""

from __future__ import print_function

import os
from collections import defaultdict

COMMANDS = "juggler-client,juggler-scheduler"
USERS = "cocaine,media-graphite-sender,www-data,unbound"


def parseline(line):
    "Parse line from ps and collect some info from /proc"

    res = defaultdict(int)

    pid, mem, utm, cmd = line.split(None, 3)
    cmd = cmd.split()[0].split("/")[-1].split(".")[0].lower().replace("-", "_")
    cmd = "".join(c for c in cmd if c.isascii() and (c.isalnum() or c == '_'))

    utm = utm.split(":")
    seconds = int(utm[1]) + int(utm[0]) * 60
    res.update({
        "mem": mem,
        "time": seconds,
        "cmd": cmd,
    })

    with open("/proc/{}/io".format(pid)) as ios:
        iostat = {v.split()[0].strip(":"): v.split()[1].strip()
                  for v in ios.readlines()}
    res.update(iostat)

    return res


def main():
    """Main function"""
    stats = defaultdict(lambda: defaultdict(int))
    for line in os.popen(
            "ps --no-headers -C {commands} -u {users} o pid,rss,bsdtime,command"
            .format(commands=COMMANDS, users=USERS)
    ):
        try:
            res = parseline(line)
        except Exception:
            continue
        for key in res.keys():
            if key == "cmd":
                continue
            try:
                val = int(res[key])
                if val == 0:
                    continue
            except ValueError:
                continue
            else:
                stats[res["cmd"]][key] += val

    for akey in stats.keys():
        for rkey in stats[akey].keys():
            item = "{}.{} {}".format(akey, rkey, stats[akey][rkey])
            print(item)


if __name__ == '__main__':
    main()
