#!/usr/bin/env python3

import logging
import os
import os.path
import sys


def output_disks(disks, file_path):
    disks.sort()

    with open(file_path, "w") as f:
        for disk in disks:
            print(disk, file=f)


path = "./scan_logs"
disk_ids = [line.rstrip() for line in sys.stdin.readlines()]
marked_dead = set([line.rstrip() for line in open(sys.argv[1]).readlines()])
marked_repaired = set([line.rstrip() for line in open(sys.argv[2]).readlines()])
deleted_disks = []
dead_disks = []
to_repair_disks = []
healthy_disks = []
misprocessed_disks = []

for disk_id in disk_ids:
    out_path = os.path.join(path, disk_id + ".out")
    err_path = os.path.join(path, disk_id + ".err")

    if disk_id in marked_dead:
        dead_disks.append(disk_id)
        continue

    try:
        with open(out_path) as out, open(err_path) as err:
            errors = err.readlines()
            processed = False
            for error in errors:
                parts = error.rstrip().split(" Error: ", 1)
                if len(parts) == 2:
                    logging.info("disk %s, last error: %s" % (disk_id, parts[1]))

                    if parts[1] == "SEVERITY_ERROR | FACILITY_SCHEMESHARD | 2 Path not found":
                        deleted_disks.append(disk_id)
                    elif parts[1] == "E_UNAUTHORIZED":
                        misprocessed_disks.append((disk_id, parts[1]))
                    else:
                        dead_disks.append(disk_id)

                    processed = True
                    break

            if processed:
                continue

            if disk_id in marked_repaired:
                to_repair_disks.append(disk_id)
                continue

            should_repair = False
            is_misprocessed = False
            for line in out.readlines():
                if not line.startswith("Bad range: "):
                    is_misprocessed = True
                    break
                should_repair = True

            if is_misprocessed:
                misprocessed_disks.append((disk_id, "Bad scanner output"))
            elif should_repair:
                to_repair_disks.append(disk_id)
            else:
                healthy_disks.append(disk_id)
    except Exception as e:
        logging.warn("failed to read logs for disk %s" % disk_id)
        misprocessed_disks.append((disk_id, str(e)))

output_disks(deleted_disks, "deleted_disks.txt")
output_disks(dead_disks, "dead_disks.txt")
output_disks(to_repair_disks, "to_repair_disks.txt")
output_disks(healthy_disks, "healthy_disks.txt")
output_disks(misprocessed_disks, "misprocessed_disks.txt")
