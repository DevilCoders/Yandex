#!/usr/bin/env python3

import json
import sys


def main():
    disk2tabs = json.load(open("existing_disks_and_tablets.json", "r"))
    tablet_ids = set([d.rstrip() for d in open(sys.argv[1]).readlines()])

    for disk_id, tabs in disk2tabs.items():
        for part_tab in tabs["Partitions"]:
            if part_tab in tablet_ids:
                print(disk_id)
                break


if __name__ == '__main__':
    main()
