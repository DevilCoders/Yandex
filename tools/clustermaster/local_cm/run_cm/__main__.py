#!/usr/bin/env python

import argparse
import os
import sys

from robot.library.yuppie.modules.cfg import Cfg
from robot.library.yuppie.modules.environment import Environment
from robot.library.yuppie.modules.cm import Cm
from robot.library.yuppie.modules.sys_mod import Sys

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Tool for configuring local Cm"
    )
    parser.add_argument(
        '-w', '--working-dir', help='Cluster master working dir',
        dest='working_dir', type=str, default=None
    )
    parser.add_argument(
        '-p', '--port-shift', help='Cluster master port shift',
        dest='port_shift', type=int, default=0
    )
    parser.add_argument(
        '-hl', '--hostlist', help='Cluster master hostlist file',
        dest='hostlist', type=str, default=None
    )
    parser.add_argument(
        '-s', '--script', help='Cluster master cm.sh script file',
        dest='script', type=str, default=None
    )
    parser.add_argument(
        '-sb', '--script-basename', help='Script (cm.sh) basename',
        dest='script_basename', type=str, default="cm.sh"
    )
    parser.add_argument(
        '-hb', '--hostlist-basename', help='Hostlist (hostlist) basename',
        dest='hostlist_basename', type=str, default="hostlist"
    )
    parser.add_argument(
        '-g', '--generate-hostlist',
        help='Generate hostlist with one host (current)',
        dest='generate_hostlist', action='store_true'
    )
    parser.add_argument(
        '--no-state-cleanup',
        help='Do not try to remove clustermaster state from previous one.',
        dest='clean_state', action='store_false'
    )
    parser.add_argument(
        '--bin-dir',
        help='Path to clustermaster\'s binaries',
        dest='bin_dir', type=str, default=None
    )
    parser.add_argument(
        '-n', '--log-number', help='Number of target logs to store',
        dest='log_number', type=int, default=3
    )

    args = parser.parse_args()
    if args.working_dir:
        Cfg().set_working_dir(args.working_dir)

    if args.hostlist:
        args.hostlist = os.path.expanduser(args.hostlist)

    if args.script:
        args.script = os.path.expanduser(args.script)

    if args.working_dir:
        args.working_dir = os.path.expanduser(args.working_dir)

    if args.bin_dir:
        args.bin_dir = os.path.expanduser(args.bin_dir)
    else:
        args.bin_dir = os.path.dirname(sys.argv[0])

    Environment()
    cm = Cm.up(
        hostlist=args.hostlist,
        cm_sh=args.script,
        port_shift=args.port_shift,
        script_basename=args.script_basename,
        hostlist_basename=args.hostlist_basename,
        generate_hostlist=args.generate_hostlist,
        clean_state=args.clean_state,
        bin_dir=args.bin_dir,
        log_number=args.log_number,
    )

    Sys.hang()
