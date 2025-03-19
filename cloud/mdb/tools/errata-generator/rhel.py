#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, unicode_literals

import json
import re
import sys

from dateutil import parser
from rpmUtils.miscutils import splitFilename

ignore = (
    "RHSA-2011:0558-01 Moderate",  # Deprecated by RHBA-2015:2018-1
    # but some versions decremented
)

with open(sys.argv[1]) as f:
    d = {}
    cur_ts = None
    cur_issue = None
    pkg_list = False
    upd_instr = False

    met = set()

    sa_pat = re.compile('.+(RHSA-[\d:\-]+)\]\s+(.+?):')
    rel_pat = re.compile('.+el(\d+)\.')
    pkg_pat = re.compile('(.+)-([\d\.]+-.+)\.(x86_64|noarch)\.rpm')
    jboss_filter_pat = re.compile('(.+?)ep\d\.el\d')

    for line in f:
        try:
            if line.startswith('Date:'):
                parsed_dt = parser.parse(' '.join(line.split()[1:]))
                offset = parsed_dt.utcoffset()
                cur_ts = int(parsed_dt.strftime('%s')) + int(offset.total_seconds())
            elif line.startswith('Subject: [RHSA-'):
                cur_issue = ' '.join(sa_pat.match(line).groups())
                if cur_issue in ignore:
                    cur_issue = None
                    continue
            elif cur_issue and 'Package List:' in line:
                pkg_list = True
            elif line == 'x86_64:\n' or line == 'noarch:\n':
                upd_instr = True
            elif upd_instr:
                if line.isspace():
                    upd_instr = False
                else:
                    match = pkg_pat.match(line)
                    if match:
                        (pkg, version, release, e, a) = \
                            splitFilename(line)
                        cur_rel = 'RHEL ' + rel_pat.match(release).groups()[0]
                        if jboss_filter_pat.match(release):
                            continue
                        if cur_rel + cur_issue + pkg in met:
                            continue
                        else:
                            met.add(cur_rel + cur_issue + pkg)
                        pkg_dict = {
                            'issue': cur_issue,
                            'pkg': pkg,
                            'version': version,
                            'release': release,
                            'timestamp': cur_ts,
                        }
                        if cur_rel in d:
                            d[cur_rel].append(pkg_dict)
                        else:
                            d[cur_rel] = [pkg_dict]
            elif 'BEGIN PGP SIGNATURE' in line:
                cur_issue = None
                pkg_list = False
                upd_instr = False
        except Exception:
            pass

    print(json.dumps(d))
