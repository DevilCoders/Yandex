#!/usr/bin/env python

from subprocess import check_output, STDOUT
from sys import exit
import re

mcelog_errs = re.compile('[un]?corrected memory errors:\s+\w+\s+\w+\s+(\d+)\sin 24h')

def m_exit(s, m):
    print("PASSIVE-CHECK:memory;%d;%s" % (s, m))
    exit(0)

def get_edac():
    data = None
    try:
        data = [el.strip() for el in check_output('/usr/bin/edac-util', stderr=STDOUT).strip().split('\n')]
    except Exception as err:
        m_exit(1, "Can't get data from edac: %s" % str(err))
    return data

def edac_errors(raw):
    errors = {}
    if "Errors" in raw[0]:
        errors = dict((el.split(':')[-2], el.split(':')[-1]) for el in raw)
    return errors


def check_mcelog():
    mcelog = None
    last_errors = 0
    try:
        mcelog = check_output(['sudo', '/usr/sbin/mcelog', '--client', '--dmi'],stderr=STDOUT).strip()
    except Exception as err:
        m_exit(1, "Can't get data from mcelog: %s" % str(err))
    if mcelog:
        mcelog_data = [ int(el) for el in re.findall(mcelog_errs, mcelog) if int(el) != 0 ]
        if mcelog_data:
            last_errors = max(mcelog_data)
    return last_errors


def main():
    status = 0
    message = "OK"
    edac = get_edac()
    if edac:
        if 'No errors to report' in edac[0]:
            m_exit(status, message)
        all_ce = None
        try:
            all_ce = edac_errors(edac)
        except Exception, err:
            m_exit(1, "Can't parse data: %s" % str(err))
        if all_ce:
            last = None
            try:
                last = check_mcelog()
            except Exception:
                pass
            if last:
                status = 2
                message = "There were %d memory errors in last 24 hours; Failed memory: %s" % \
                          (last, ','.join('%s: %s' % (k,v) for k, v in all_ce.items()))
            else:
                status = 2
                message = "Failed memory: %s" % (','.join('%s: %s' % (k,v) for k, v in all_ce.items()))

    m_exit(status, message)

if __name__ == '__main__':
    main()
    exit(0)


