#!/usr/bin/env python3

'''
CLOUD-63374
ядро: 4.19-yc3
размер HP: 2M
аллокация: в cmdline(размер, кол-во), учесть   CLOUD-60666 Новый kernel: cmdline: nx_huge_pages должен выставляться в auto
монтирование: c размером и параметрами в отдельный endpoint
оставляем под систему 48Гб обычных, остальное в HP

cat /proc/meminfo
MemTotal:       16305832 kB
MemFree:         4643352 kB
MemAvailable:    8277264 kB
Buffers:          448276 kB
Cached:          4765384 kB
SwapCached:        36648 kB
Active:          6758328 kB
Inactive:        4395316 kB
Active(anon):    5303576 kB
Inactive(anon):  2108784 kB
Active(file):    1454752 kB
Inactive(file):  2286532 kB
Unevictable:         732 kB
Mlocked:             732 kB
SwapTotal:      15625212 kB
SwapFree:       15567860 kB
Dirty:              4060 kB
Writeback:             0 kB
AnonPages:       5882268 kB
Mapped:          1441916 kB
Shmem:           1475760 kB
Slab:             327988 kB
SReclaimable:     233060 kB
SUnreclaim:        94928 kB
KernelStack:       13536 kB
PageTables:        48592 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:    23778128 kB
Committed_AS:   13878044 kB
VmallocTotal:   34359738367 kB
VmallocUsed:           0 kB
VmallocChunk:          0 kB
Percpu:             1240 kB
HardwareCorrupted:     0 kB
AnonHugePages:   1320960 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:               0 kB
DirectMap4k:      714056 kB
DirectMap2M:    12795904 kB
DirectMap1G:     3145728 kB
'''

import sys
import logging
import argparse
import re
import pprint

logging.basicConfig(level=logging.ERROR, format='%(levelname)s - %(message)s')

SERVICES_MEMORY_BYTES = 50331648  # kB == 48 GB, 8388608 - 8Gb
DEFAULT_HP_SIZE = 2048  # kB
MEMINFO_REGEX = re.compile(r'^(?P<key>[^:]+):\ +(?P<value>\d+)(\ (?P<dimension>\w+))?$')
MEMINFO_FILE = "/proc/meminfo"


class UnexpectedDimension(Exception):
    def __init__(self, expected, getted):
        super().__init__('''Get dimension "{}", but expected "{}"'''.format(getted, expected))


def parse_args():
    parser = argparse.ArgumentParser(description='Isolate threads')
    parser.add_argument('-l', '--level', action='store',
                        help='Logging level, default: {}'.format(logging.getLevelName(logging.root.level)),
                        choices=['CRITICAL', 'DEBUG', 'ERROR', 'FATAL', 'INFO', 'NOTSET', 'WARN', 'WARNING']
                        )
    parser.add_argument('-d', '--dryrun', action='store_true', help='Do not provide CMD output, set exit code to 2')
    parser.add_argument('-s', '--hpsize', action='store', type=int,
                        help='Expected HugePage size in kB, default: {} kB'.format(DEFAULT_HP_SIZE),
                        default=DEFAULT_HP_SIZE)
    parser.add_argument('-c', '--hpmincnt', action='store', type=int,
                        help='Custom HP number')
    parser.add_argument('-t', '--strict', action='store_true', help='Fail, if HugePage size not expected')
    parser.add_argument('-m', '--commonmem', action='store', type=int,
                        help='Memory size to preserve as common, non-HP, in kB, default: {} GB'.format(
                            SERVICES_MEMORY_BYTES / 1024 / 1024),
                        default=SERVICES_MEMORY_BYTES)
    args = parser.parse_args()
    return args


def parse_meminfo():
    res = {}
    with open(MEMINFO_FILE, 'r') as meminfofile:
        for line in meminfofile:
            parsed = MEMINFO_REGEX.match(line).groupdict()
            if parsed['dimension'] and parsed['dimension'] != 'kB':
                logging.critical('Get dimension "%s", but expected "%s"', parsed['dimension'], 'kB')
                return None
            res[parsed['key']] = int(parsed['value'])
    logging.debug(pprint.pformat(res))
    return res


def exit_script(code):
    logging.debug('Exit code: %d', code)
    sys.exit(code)


def main():
    args = parse_args()
    if args.level:
        logging.root.setLevel(getattr(logging, args.level))
    try:
        meminfo = parse_meminfo()
        for key in ('MemTotal', 'Hugepagesize'):
            assert key in meminfo, key
    except AssertionError:
        logging.critical('In meminfo expected keys was not found: %r', key)
        exit_script(1)
    except Exception as ex:
        logging.critical('Error while parsing meminfo %r', ex)
        exit_script(1)
    if meminfo['Hugepagesize'] != args.hpsize and args.strict:
        logging.error('Unexpected HugePage size. Expected: %d, actual: %d', args.hpsize, meminfo['Hugepagesize'])
        exit_script(1)
    hp_memory = meminfo['MemTotal'] - args.commonmem
    max_hp_number = hp_memory // meminfo['Hugepagesize']
    current_hp_number = max_hp_number
    if args.hpmincnt:
        current_hp_number = args.hpmincnt
    if max_hp_number < args.hpmincnt:
        logging.error('Not enouth memory for HugePages.')
        exit_script(2)
    elif args.dryrun:
        logging.info('Dry run')
        exit_script(2)
    else:
        print('hugepagesz={size}K hugepages={number}'.format(size=meminfo['Hugepagesize'], number=current_hp_number))
        exit_script(0)


if __name__ == '__main__':
    main()
