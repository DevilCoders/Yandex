#!/usr/bin/env python3

import re
import subprocess
import sys


PACKAGE_NAME = 'puncher-a'
DEBSUMS_EXECUTABLE = '/usr/bin/debsums'
DEBSUMS_ARGS = (PACKAGE_NAME, )



def get_debsums_diffs():
    sp = subprocess.Popen(
        (DEBSUMS_EXECUTABLE, ) + DEBSUMS_ARGS,
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    sp.wait(120)

    # Typical output is:
    # /usr/share/doc/clickhouse-client/copyright                      FAILED
    res = []
    for line in sp.stdout:
        if line.endswith(b'FAILED\n'):
            res.append(' '.join(line.decode().strip().split()))
    return res


if __name__ == "__main__":
    bad_checksums = get_debsums_diffs()

    if bad_checksums:
        print('PASSIVE-CHECK:puncher_checksum;2;Puncher bad checksum %s' % bad_checksums)
    else:
        print('PASSIVE-CHECK:puncher_checksum;0;OK')
