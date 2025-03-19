#!/usr/bin/env python3

import json
import os
import sys

hostname = os.getenv("KMS_HOST_NAME")

update_packages_filename = os.path.join(os.path.dirname(sys.argv[0]), "packages-to-update.json")

cmdline = "pssh run \"sudo apt update\" {0}".format(hostname)
print("> Running " + cmdline)
if os.system(cmdline) != 0:
    print("> ERROR: Command failed")

with open(update_packages_filename) as f:
    packages_ver = json.load(f)
    packages_str = " ".join("{0}={1}".format(p, v) for p, v in packages_ver.items())
    cmdline = "pssh run \"sudo apt install -y {0}\" {1}".format(packages_str, hostname)
    print("> Running " + cmdline)
    if os.system(cmdline) != 0:
        print("> ERROR: Command failed")
