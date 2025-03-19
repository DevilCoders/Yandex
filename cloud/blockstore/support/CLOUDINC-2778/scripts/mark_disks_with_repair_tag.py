#!/usr/bin/env python3

import json
import subprocess
import sys


def main():
    disk2tabs = json.load(open("existing_disks_and_tablets.json", "r"))
    ids = set([d.rstrip() for d in open(sys.argv[1]).readlines()])
    already_marked = set([d.rstrip() for d in open(sys.argv[2]).readlines()])

    for disk_id, tabs in disk2tabs.items():
        if disk_id in already_marked:
            continue

        found = disk_id in ids

        if not found:
            for part_tab in tabs["Partitions"]:
                if part_tab in ids:
                    found = True
                    break

        if not found:
            continue

        try:
            cmd = [
                'blockstore-client',
                'executeaction',
                '--action', 'modifytags',
                '--input-bytes', '{"DiskId":"%s", "TagsToAdd": ["repair"]}' % disk_id,
                '--iam-token-file', 'tok',
            ]
            # print(" ".join(cmd))
            # result = ""
            result = subprocess.check_output(cmd, stderr=subprocess.PIPE).decode("utf-8")
        except subprocess.CalledProcessError as e:
            print("error processing disk %s: %s" % (disk_id, e))

        print("modifytags %s %s result: %s" % (disk_id, part_tab, result), file=sys.stderr)
        print(disk_id)


if __name__ == '__main__':
    main()
