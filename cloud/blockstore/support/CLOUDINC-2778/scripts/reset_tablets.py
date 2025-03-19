#!/usr/bin/env python3

import json
import subprocess
import sys


def main():
    disk2tabs = json.load(open("existing_disks_and_tablets.json", "r"))
    disk_ids = [d.rstrip() for d in open(sys.argv[1]).readlines()]
    manually_marked_dead = [d.rstrip() for d in open(sys.argv[2]).readlines()]

    for disk_id in disk_ids:
        if disk_id in manually_marked_dead:
            continue

        tabs = disk2tabs.get(disk_id)
        if tabs is None:
            print("tabs for disk %s not found" % disk_id, file=sys.stderr)
            continue

        for part_tab in tabs["Partitions"]:
            try:
                cmd = [
                    'blockstore-client',
                    'executeaction',
                    '--action', 'resettablet',
                    '--input-bytes', '{"TabletId":"%s", "Generation": 0}' % part_tab,
                    '--iam-token-file', 'tok',
                ]
                # print(" ".join(cmd))
                # result = ""
                result = subprocess.check_output(cmd, stderr=subprocess.PIPE).decode("utf-8")
            except subprocess.CalledProcessError as e:
                print("error processing disk %s: %s" % (disk_id, e))

            print("reset tab %s %s result: %s" % (disk_id, part_tab, result))


if __name__ == '__main__':
    main()
