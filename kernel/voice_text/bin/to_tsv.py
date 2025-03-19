#!/usr/bin/env python
from __future__ import print_function
import sys
import json

def remove_hilight(s):
    return s.replace(u"\007]", "").replace(u"\007[", "")

for line in sys.stdin:
    data = json.loads(line.strip().split('\t')[-1])
    if "fact_text" not in data:
        continue
    elif type(data["fact_text"]) == unicode:
        text = remove_hilight(data["fact_text"]).encode("utf8")
    else:
        continue
    print(data["corrected_query"].encode("utf8"), text, data["fact_source"], sep="\t")
