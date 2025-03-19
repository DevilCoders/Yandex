#!/usr/bin/env python

import argparse
import re
import sys

def die(code=0, comment='OK'):
    print('{code};{comment}'.format(code=code, comment=comment))
    sys.exit(0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--warn', default=50, help='Warning used/total percent')
    parser.add_argument('-c', '--crit', default=90, help='Critical used/total percent')
    args = parser.parse_args()

    try:
        values = {}
        with open('/proc/meminfo', 'r') as meminfo:
            for row in meminfo.read().split('\n'):
                if row:
                    key, value = row.split(':')
                    key, value = key.strip(), value.strip()
                    if value.endswith(' kB'):
                        value = value[:-3]
                    values[key] = int(value)
        result = 100 * (values['MemTotal'] - values['MemFree'] - values['Buffers'] - values['Cached']) / values['MemTotal']
        msg = 'memory usage %s%%' % result
        if result > args.crit:
            die(2, msg)
        elif result > args.warn:
            die(1, msg)
        else:
            die(0, msg)
    except Exception as e:
        exp = str(e).encode('string_escape')[:350]
        die(1, '{exc}: {value}'.format(exc=type(e).__name__, value=exp))

    die()


if __name__ == '__main__':
    main()

