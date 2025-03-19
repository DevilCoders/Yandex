#!/usr/bin/env python3

import json
import subprocess


def main():
    disk2tabs = {}
    disk_ids = [d.rstrip() for d in open("all_disk_ids.txt", "r").readlines()]
    i = 0

    for disk_id in disk_ids:
        try:
            result = str(subprocess.check_output([
                'blockstore-client',
                'executeaction',
                '--action', 'describevolume',
                '--input-bytes', '{"DiskId":"%s"}' % disk_id,
                '--iam-token-file', 'tok',
                '--timeout', '4',
                ], stderr=subprocess.PIPE).decode("utf8").rstrip())
        except subprocess.CalledProcessError as e:
            print("error processing disk %s: %s" % (disk_id, e))
            continue

        try:
            j = json.loads(result)
        except Exception as e:
            print(result)
            raise e
        disk_info = disk2tabs[disk_id] = {}
        disk_info["Volume"] = str(j["VolumeTabletId"])
        disk_info["Partitions"] = []
        for partition in j.get("Partitions", []):
            disk_info["Partitions"].append(str(partition["TabletId"]))

        i += 1
        print("processed %s/%s disks" % (i, len(disk_ids)))

    open("existing_disks_and_tablets.json", 'w').write(json.dumps(disk2tabs, indent=4))

if __name__ == '__main__':
    main()
