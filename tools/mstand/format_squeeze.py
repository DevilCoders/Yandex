#!/usr/bin/env python3

import json
import sys
# noinspection PyUnresolvedReferences
import urllib.parse


import yaqutils.time_helpers as utime

for line in open(sys.argv[1]):
    data = json.loads(line)
    time = utime.timestamp_to_datetime_msk(data["ts"])
    time_str = time.strftime("%d.%m.%Y %H:%M.%S")
    # noinspection PyUnresolvedReferences
    print("[{}] ".format(time_str), end="")
    if data["type"] == "request":
        # noinspection PyUnresolvedReferences
        print("query: {query}".format(**data), end="")
        if data["query"] != data["correctedquery"]:
            print(", corrected: {correctedquery}".format(**data))
        else:
            print()
    elif data["type"] == "click":
        # noinspection PyUnresolvedReferences
        data["url"] = urllib.parse.unquote(data["url"])
        print("click url: {url}".format(**data))

