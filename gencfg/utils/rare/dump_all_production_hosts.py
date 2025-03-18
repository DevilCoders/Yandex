#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import re
from argparse import ArgumentParser

import gencfg
from core.db import DB
from utils.pregen.get_extra_bot_machines import DEFAULT_FILTER
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Dump all production hosts for specified tag")
    parser.add_argument("-d", "--db-path", type=argparse_types.dirpath, required=True,
                        help="Obligatory. Directory with db")
    parser.add_argument("-o", "--out-dir-path", type=argparse_types.dirpath, required=True,
                        help="Obligatory. Output directory")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    mydb = DB(options.db_path)

    flt = re.compile(DEFAULT_FILTER)

    allhosts = filter(lambda x: re.match(flt, x.name) is not None, mydb.hosts.get_all_hosts())
    allhosts.sort(cmp=lambda x, y: cmp(x.name, y.name))

    dt = mydb.get_repo().get_last_commit().date

    fname = os.path.join(options.out_dir_path, '%s' % dt)
    with open(fname, 'w') as f:
        for host in allhosts:
            f.write("%s %s %s %s\n" % (host.name.split('.')[0], host.model, host.memory, host.power))


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
