#!/usr/bin/env python
# -*- coding: utf-8 -*-
import subprocess 

# 0 is OK, 1 is WARN and 2 is CRIT
STATUS_MAP = {"OK": 0,
              "RECOVERY": 0,
              "WARNING": 1,
              "NOTSET": 1,
              "FAILED": 2,
              "UNKNOWN": 2}

def get_clock_status():
    try:
        command = ["sudo", "-u",
                   "hw-watcher", "/usr/sbin/hw_watcher",
                   "link", "status"]
        output = subprocess.check_output(command).replace("\n", "").split(";")
        status, msg = output[0], '. '.join(output[1:])
    except Exception:
        return 1, "Can't get status from hw-watcher link"
    
    mapped_status = STATUS_MAP.get(status)

    if mapped_status is not None:
        return mapped_status, msg
    else:
        # If any new status will be added – link to wiki page could be useful
        return 2, "Status {} not in script map. Map is here nda.ya.ru/3VspUi".format(status)


def main():
    status, msg = get_clock_status()
    print("PASSIVE-CHECK:hw_watcher_link;{0};{1}".format(status, msg))

if __name__ == "__main__":
    main()
