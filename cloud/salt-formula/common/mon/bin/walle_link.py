#!/usr/bin/env python

# Provides: walle_link

from __future__ import unicode_literals

import json
import subprocess


def get_link_status():
    cmd = "sudo -u hw-watcher /usr/sbin/hw_watcher link extended_status"

    process = subprocess.Popen(cmd.split(), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               close_fds=True)
    stdout, stderr = process.communicate()

    if process.returncode or stderr:
        error = stderr.strip() or stdout.strip()
        raise Exception(error.decode("utf-8"))

    return json.loads(stdout)


def run_check():
    try:
        result = get_link_status()
        status = {"OK": 0, "FAILED": 2}.get(result["status"], 1)
    except Exception as e:
        return 1, {"reason": "Can't get status from hw-watcher: {}".format(e)}
    else:
        return status, {"result": result}


def main():
    status, metadata = run_check()
    print("PASSIVE-CHECK:walle_link;{};{}".format(status, json.dumps(metadata)))


if __name__ == "__main__":
    main()
