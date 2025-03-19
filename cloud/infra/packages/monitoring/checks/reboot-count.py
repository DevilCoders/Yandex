#!/usr/bin/env python3

from datetime import datetime
from yc_monitoring import JugglerPassiveCheck
from ycinfra import Popen

HOURS_FOR_CRIT = 1
HOURS_FOR_WARN = 24


def check_reboot(check: JugglerPassiveCheck):
    command = "last -F"
    try:
        returncode, stdout, stderr = Popen().exec_command(command)
        if returncode != 0:
            raise Exception("Command: {}. ret_code: {}. STDERR: {}".format(command, returncode, stderr))
    except Exception as ex:
        check.crit("Error reading lastlog: {}".format(ex))
        return

    crits = 0
    warns = 0
    now = datetime.now()

    for line in filter(lambda x: x.startswith("reboot"), stdout.split('\n')):
        date = ' '.join(line.split()[4:9])
        try:
            date = datetime.strptime(date, "%c")
        except Exception as ex:
            check.crit("Error parsing date {}: {}".format(date, ex))
            continue

        diff_hours = (now - date).total_seconds() / (60.0 * 60)
        if diff_hours < HOURS_FOR_CRIT:
            crits += 1
        elif diff_hours < HOURS_FOR_WARN:
            warns += 1

    if crits:
        check.crit("Server was rebooted {} times in last {} hours, {} times in last {} hours".format(
            crits, HOURS_FOR_CRIT, warns + crits, HOURS_FOR_WARN
        ))
    elif warns:
        check.warn("Server was rebooted {} times in last {} hours".format(warns, HOURS_FOR_WARN))


def main():
    check = JugglerPassiveCheck("reboot-count")
    try:
        check_reboot(check)
    except Exception as ex:
        check.crit("During check exception raised: ({}): {}".format(ex.__class__.__name__, ex))
    print(check)


if __name__ == '__main__':
    main()
