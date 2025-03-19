#!/usr/bin/env python3
import os.path
import shlex
import subprocess

from yc_monitoring import JugglerPassiveCheck

CMD_STR = '/usr/bin/mdevices_info'
CMD = shlex.split(CMD_STR)
TABLE_SEPARATOR = '------------'


def main():
    check = JugglerPassiveCheck('nic_health')
    try:
        proc = subprocess.run(CMD,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              universal_newlines=True)
        if proc.returncode != 0:
            check.crit("`{}` exit code {}, stderr: {}".format(CMD_STR, proc.returncode, proc.stderr))
            print(check)
            return
        output = proc.stdout.splitlines()
        table = output[output.index(TABLE_SEPARATOR) + 1:]
        if len(table) == 0:
            check.crit('`{}` returned an invalid table: {}'.format(CMD_STR, proc.stdout))
        for row in table:
            if len(row.split()) != 2:
                check.crit('`{}` returned an invalid table: {}'.format(CMD_STR, proc.stdout))
                break
            _, mst = row.split()
            if not os.path.exists(mst):
                check.crit('`{}` returned an invalid table: {}'.format(CMD_STR, proc.stdout))
                break
    except Exception as e:
        check.crit('An error occurred during check: ({}): {}'.format(e.__class__.__name__, e))
    print(check)


if __name__ == '__main__':
    main()
