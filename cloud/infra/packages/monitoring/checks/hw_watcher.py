#!/usr/bin/env python3
"""
This is common check for all juggler checks based on hw-watcher module status.
It requires hw-watcher module as an argument.
Possible check names: "bmc", "bmcc", "clock", "cpu", "disk", "ecc", "gpu", "infiniband", link", "mem", "metr"]
"""
import sys
from ycinfra import Popen

# 0 is OK, 1 is WARN and 2 is CRIT
STATUS_MAP = {"OK": 0,
              "RECOVERY": 0,
              "WARNING": 1,
              "NOTSET": 1,
              "FAILED": 2,
              "UNKNOWN": 2}


def get_hw_watcher_status(hw_watcher_check_name):
    try:
        command = "sudo -u hw-watcher /usr/sbin/hw_watcher {} status".format(hw_watcher_check_name)
        returncode, stdout, stderr = Popen().exec_command(command)
        if returncode != 0:
            raise Exception("Command: {}. ret_code: {}. STDERR: {}".format(command, returncode, stderr))
        output = stdout.replace("\n", "").split(";")
        status, msg = output[0], '. '.join(output[1:])
    except Exception as ex:
        return 1, "Can't get status from hw-watcher {}: {}".format(hw_watcher_check_name, ex)

    mapped_status = STATUS_MAP.get(status)

    if mapped_status is not None:
        return mapped_status, msg
    else:
        # If any new status will be added – link to wiki page could be useful
        return 2, "Status {} not in script map. Map is here nda.ya.ru/3VspUi".format(status)


def main():
    try:
        hw_watcher_check_name = sys.argv[1]
    except IndexError:
        raise IndexError("Expected hw-watcher check name as an argument")
    check_name = "hw_watcher_{}".format(hw_watcher_check_name)
    status, msg = get_hw_watcher_status(hw_watcher_check_name)
    print("PASSIVE-CHECK:{};{};{}".format(check_name, status, msg))


if __name__ == "__main__":
    main()
