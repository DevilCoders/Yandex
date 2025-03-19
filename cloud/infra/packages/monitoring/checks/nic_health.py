#!/usr/bin/env python3
import os.path
from ycinfra import Popen

from yc_monitoring import JugglerPassiveCheck

CMD_STR = '/usr/bin/mdevices_info'
TABLE_SEPARATOR = '------------'


def main():
    check = JugglerPassiveCheck('nic_health')
    try:
        returncode, stdout, stderr = Popen().exec_command(CMD_STR)
        if returncode != 0:
            check.crit("`{}` exit code {}, stderr: {}".format(CMD_STR, returncode, stderr))
            print(check)
            return
        output = stdout.splitlines()
        table = output[output.index(TABLE_SEPARATOR) + 1:]
        if len(table) == 0:
            check.crit('`{}` returned an invalid table: {}'.format(CMD_STR, stdout))
        for row in table:
            if len(row.split()) != 2:
                check.crit('`{}` returned an invalid table: {}'.format(CMD_STR, stdout))
                break
            _, mst = row.split()
            if not os.path.exists(mst):
                check.crit('`{}` returned an invalid table: {}'.format(CMD_STR, stdout))
                break
    except Exception as e:
        check.crit('An error occurred during check: ({}): {}'.format(e.__class__.__name__, e))
    print(check)


if __name__ == '__main__':
    main()
