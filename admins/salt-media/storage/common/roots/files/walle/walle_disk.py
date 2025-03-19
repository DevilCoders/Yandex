#!/usr/bin/env python

# Provides: walle_disk

import copy
import json
import subprocess


def list_disks():
    disks = []

    with open("/proc/diskstats") as diskstats:
        for line in diskstats:
            line = line.strip()
            if not line:
                continue

            line_arr = line.split()
            if len(line_arr) < 3:
                continue

            if line_arr[2].startswith("loop") or line_arr[2].startswith("ram"):
                continue

            disks.append(line_arr[2])

    return disks


def get_disk_info(disk):
    if disk.startswith("sd"):
        return "{}: single disk mounted".format(disk)

    if disk.startswith("md"):
        with open("/proc/mdstat") as mdstat:
            for line in mdstat:
                if line.startswith(disk):
                    line_arr = line.split()
                    if line_arr[3] == "raid0":
                        return "{}: raid0 mounted; raid disks: {}".format(disk, line_arr[4:])

    return ""


def check_disks(disks):
    with open("/proc/mounts") as mounts:
        for line in mounts:
            line = line.strip()
            if not line:
                continue

            line_arr = line.split()
            mount_disk = line_arr[0]
            if not mount_disk.startswith("/dev/"):
                continue

            if line_arr[3].startswith("ro"):
                for disk in disks:
                    if mount_disk[-len(disk):] == disk:
                        error = "{} is read-only".format(line_arr[1])

                        disk_info = get_disk_info(disk)
                        if disk_info:
                            error += "; " + disk_info

                        return error


def get_hw_watcher_status():
    cmd = "/usr/sbin/hw_watcher disk extended_status"

    process = subprocess.Popen(cmd.split(), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               close_fds=True)
    stdout, stderr = process.communicate()

    if process.returncode or stderr:
        error = stderr.strip() or stdout.strip()
        raise Exception(error.decode("utf-8")[:800])

    return json.loads(stdout)


def get_juggler_description(metadata):
    # Juggler limits description length to 1000 bytes, so try to save space here by dropping non-sensitive data

    metadata = copy.deepcopy(metadata)

    while True:
        description = json.dumps(metadata)
        if len(description) <= 1000:
            return description

        try:
            result = metadata["result"]

            if result.pop("failed_disks", None) is not None:
                continue

            reason = result["disk2replace"]["reason"]
            if len(reason) > 1:
                reason.pop(0)
                continue

            reason = result.get("reason", [])
            if len(reason) > 1:
                reason.pop(0)
                continue
        except Exception:
            pass

        return description


def run_check():
    # try:
    #     disk_error = check_disks(list_disks())
    # except Exception as e:
    #     return 1, {"reason": "Can't check disks for errors: {}".format(e)}

    # if disk_error is not None:
    #     return 2, {"reason": disk_error}

    try:
        result = get_hw_watcher_status()
        status = {"OK": 0, "WARNING": 0, "FAILED": 2, "UNKNOWN": 2}.get(result["status"], 1)
    except Exception as e:
        return 1, {"reason": "Can't get status from hw-watcher: {}".format(e)}
    else:
        return status, {"result": result}


def main():
    status, metadata = run_check()
    print("PASSIVE-CHECK:walle_disk;{};{}".format(status, get_juggler_description(metadata)))


if __name__ == "__main__":
    main()
