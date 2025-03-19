#!/usr/bin/env python

# Provides: walle_memory

import json
import subprocess


def get_hw_watcher_status(check_type):
    process = subprocess.Popen(
        ["/usr/sbin/hw_watcher", check_type, "extended_status"],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
    stdout, stderr = process.communicate()

    if process.returncode or stderr:
        error = stderr.strip() or stdout.strip()
        raise Exception(error.decode("utf-8")[:800])

    return json.loads(stdout)


def run_check():
    try:
        results = {
            "mem": get_hw_watcher_status("mem"),
            "ecc": get_hw_watcher_status("ecc"),
        }

        if any(result["status"] == "FAILED" for result in results.itervalues()):
            status = 2
        elif all(result["status"] == "OK" for result in results.itervalues()):
            status = 0
        else:
            status = 1

        return status, {"results": results}
    except Exception as e:
        return 1, {"reason": "Can't get status from hw-watcher: {}".format(e)}


def main():
    status, metadata = run_check()
    print("PASSIVE-CHECK:walle_memory;{};{}".format(status, json.dumps(metadata)))


if __name__ == "__main__":
    main()
