#!/usr/bin/env python3

import argparse
import json
import os
import requests


api_url = "https://solomon.yandex.net/api/v2/projects/nbs/sensors/data"


def fetch_scalar(aggr_mode, service, disk_id, sensor, aux=[]):
    headers = {
        "Authorization": "OAuth " + os.environ["TOKEN"],
        "Content-Type": "application/json",
    }
    aux_str = "," + ",".join("%s=%s" % (x[0], x[1]) for x in aux) if len(aux) else ""
    if disk_id.startswith("fhm"):
        cluster_tag = "vla"
    elif disk_id.startswith("epd"):
        cluster_tag = "sas"
    elif disk_id.startswith("ef3"):
        cluster_tag = "myt"
    else:
        raise Exception("weird disk_id: %s" % disk_id)
    data = {
        "program": "%s({cluster=yandexcloud_prod_%s,service=%s,host=cluster,volume=%s,sensor=%s%s})" % (
            aggr_mode,
            cluster_tag,
            service,
            disk_id,
            sensor,
            aux_str
        ),
        "from": "2021-03-20T13:30:00.000Z",
        #"to": "2021-08-21T13:30:00.000Z",
        "to": "2021-10-20T13:30:00.000Z",
    }
    # print(json.dumps(data))
    r = requests.post(api_url, headers=headers, data=json.dumps(data))
    result = r.json()
    if "scalar" not in result:
        if result.get("type") != "TOO_MANY_LINES_IN_AGGREGATION":
            # print(json.dumps(result))
            r.raise_for_status()
        return None
    scalar = result["scalar"]
    return scalar if scalar != "NaN" else None


class Disk(object):

    def __init__(self, disk_id, cloud_id, cloud_info, raw):
        self.disk_id = disk_id
        self.cloud_id = cloud_id
        self.cloud_info = cloud_info
        self.raw = raw


parser = argparse.ArgumentParser()
parser.add_argument("--disks-with-alien-data", help="disks with alien data", required=True)

args = parser.parse_args()

disks = []
for line in open(args.disks_with_alien_data).readlines():
    line = line.rstrip()
    parts = line.split("\t")
    disks.append(Disk(parts[1][7:], parts[2][8:], parts[3], line))

for disk in disks:
    written = fetch_scalar(
        "integrate",
        "server_volume",
        disk.disk_id,
        "RequestBytes",
        [("request", "WriteBlocks")]
    )
    zeroed = fetch_scalar(
        "integrate",
        "server_volume",
        disk.disk_id,
        "RequestBytes",
        [("request", "ZeroBlocks")]
    )
    size = fetch_scalar(
        "max",
        "service_volume",
        disk.disk_id,
        "BytesCount"
    )
    no_data = written is None or zeroed is None or size is None
    if no_data:
        written = 0
        zeroed = 0
        size = 0
    else:
        written = written / 1024 / 1024 / 1024
        zeroed = zeroed / 1024 / 1024 / 1024
        size = size / 1024 / 1024 / 1024

    print("%s\tWritten=%sGiB,Zeroed=%sGiB,Size=%sGiB\t%s" % (
        disk.raw,
        written,
        zeroed,
        size,
        "NO_DATA" if no_data else "OK" if (written + zeroed) >= size else "SUSPICIOUS"
    ))

