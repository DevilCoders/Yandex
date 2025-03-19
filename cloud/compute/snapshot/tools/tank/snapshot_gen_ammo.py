#!/usr/bin/python

import sys
import json
import time
import random

CHUNKS = 512

projects = ["tank-{}".format(i) for i in xrange(16)]
methods = [
    "create_base",
    "convert",
    "list",
    "list_full",
    "info"
]
sort = ["created", "description", "real_size"]

def get_list_full_params():
    d = {}
    if random.random() > 0.25:
        d["project"] = random.choice(projects)
        if random.random() > 0.3:
            d["search_full"] = "nk-"
        if random.random() > 0.3:
            d["created_before"] = True
    return d

def get_list_params():
    d = get_list_full_params()
    d["limit"] = 100
    if random.random() > 0.5:
        if random.random() > 0.5:
            d["sort"] = random.choice(sort)
        else:
            d["sort"] = "-" + random.choice(sort)
    return d

if len(sys.argv) > 2:
    methods = sys.argv[2].split(",")


with open("ammo.txt", "w") as f:
    for i in xrange(int(sys.argv[1])):
        method = random.choice(methods)
        if method == "create_base":
            missile = json.dumps({"size": CHUNKS})
        elif method == "convert":
            missile = json.dumps({"bucket": "http://sasd3-c106-3.cloud.yandex.net:8080/images", "key": "twitteros.img"})
        elif method == "list":
            missile = json.dumps(get_list_params())
        elif method == "list_full":
            missile = json.dumps(get_list_full_params())
        elif method == "info":
            missile = json.dumps({})

        f.write("{}\t{}\n".format(method, missile))
