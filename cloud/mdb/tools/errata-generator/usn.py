#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, unicode_literals

import json
import sys

from dateutil import parser

with open(sys.argv[1]) as f:
    d = {}
    cur_issue = None
    cur_ts = None
    cur_rel = None
    upd_instr = False

    met = set()

    for line_enc in f:
        line = line_enc.decode('utf-8')
        if line.startswith('Date:'):
            parsed_dt = parser.parse(' '.join(line.split()[1:]))
            offset = parsed_dt.utcoffset()
            cur_ts = int(parsed_dt.strftime('%s')) + int(offset.total_seconds())
        elif line.startswith('Ubuntu Security Notice USN-'):
            cur_issue = line.split()[-1].rstrip()
        elif line == 'Update instructions:\n':
            upd_instr = True
        elif upd_instr and not line.isspace():
            if line.startswith('Ubuntu '):
                cur_rel = line.replace(':\n', '')
            elif line[0].isspace():
                try:
                    pkg, version = line.lstrip().rstrip().split()
                    if cur_issue + pkg + cur_rel not in met:
                        met.add(cur_issue + pkg + cur_rel)
                        pkg_dict = {
                            'issue': cur_issue,
                            'pkg': pkg,
                            'version': version,
                            'timestamp': cur_ts,
                        }
                        if cur_rel in d:
                            d[cur_rel].append(pkg_dict)
                        else:
                            d[cur_rel] = [pkg_dict]
                except Exception:
                    upd_instr = False
        elif upd_instr and line == 'References:\n':
            upd_instr = False

    print(json.dumps(d))
