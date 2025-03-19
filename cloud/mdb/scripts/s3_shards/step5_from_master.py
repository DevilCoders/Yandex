#!/usr/bin/env python3

# https://wiki.yandex-team.ru/users/annkpx/naklikivanie-novyx-shardov-s3/#5.sobstvennonalivka

import os
import pdb
import re

cc = {'h', 'i', 'k'}

with open('masters.list') as f:
    for line in f:
        host = line.strip()

        """
        Host format: s3db[n][c].db.yandex.net
        Example: s3db43h.db.yandex.net
        """
        match = re.search('s3db([0-9]+)([h,i,k]).db.yandex.net', host)
        if not match:
            print(f'Host {host} does not match host format')
            pdb.set_trace()

        n = match.group(1)
        c = match.group(2)

        slaves = [f's3db{n}{x}.db.yandex.net' for x in (cc - {c})]
        print(line.strip(), '->', slaves[0], slaves[1])

        for slave in slaves:
            ret = os.system(f'ssh -A root@{slave} "rm -f /tmp/.pgsync_rewind_fail.flag && '
                            'service pgsync stop && '
                            '(service pgbouncer stop || service odyssey stop) &&'
                            'service postgresql stop && rm -rf /var/lib/postgresql/12/data &&'
                            'rm -rf /etc/postgresql/12/data"')
            print('Code:', ret)
            if ret != 0:
                pdb.set_trace()

            cmd = '''timeout 86370 salt-call state.highstate queue=True pillar=\\"{{'pg-master': '{host}'}}\\"'''
            cmd = f'ssh -A root@{slave} "{cmd.format(host=host)}"'
            print('Run cmd:', cmd)
            ret = os.system(cmd)

            print('Code:', ret)
            if ret != 0:
                pdb.set_trace()

            print('Success on', slave)
