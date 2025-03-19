#!/usr/bin/env python3

import csv
import json
import os
import sys
from packaging import version  # you may need to install packaging via:  pip3 install packaging

if len(sys.argv) != 2:
    print("Usage:")
    print("  {0} file-from-schecker-ticket.csv".format(sys.argv[0]))
    sys.exit(1)
csv_path = sys.argv[1]

update_packages_filename = os.path.join(os.path.dirname(sys.argv[0]), "packages-to-update.json")

with open(update_packages_filename) as f:
    packages = json.load(f)

for r in csv.DictReader(open(csv_path)):
    pkg_name = r['pkg_name']
    new_ver = r['pkg_fixed_ver']
    if pkg_name in packages:
        if version.parse(packages[pkg_name]) > version.parse(new_ver):
            print("Skipping version {0} for {1}, already has newer version {2}".format(new_ver, pkg_name, packages[pkg_name]))
            continue
    packages[pkg_name] = new_ver

with open(update_packages_filename, "w") as f:
    json.dump(packages, f, indent=2, sort_keys=True)
