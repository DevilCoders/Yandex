#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import sys

res = {}

for i in sys.argv[1:]:
    with open(i) as f:
        try:
            d = json.loads(f.read())

            for j in d:
                if j in res:
                    res[j] += d[j]
                else:
                    res[j] = d[j]
        except Exception:
            pass

print(json.dumps(res, sort_keys=True))
