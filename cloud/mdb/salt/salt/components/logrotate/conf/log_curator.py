#!/usr/bin/env python3
"""
This script keeps track of how much space the
logs occupy, even compressed.

Designed for use with logrotate, which is unable to calculate
a combined size of all logs.
"""
import argparse
import sys
import re
import os
import shutil
import logging.handlers
from os.path import (getsize, getmtime, join, islink)

NEWEST_TS = float(2 ** 31)  # *Signed int 32 max*
SIZE_SCALE_FACTOR = 2  # Additional space reserved for logrotate

LOG = logging.getLogger('log-curator')
logging.basicConfig(
    level=logging.DEBUG,
    handlers=[logging.handlers.SysLogHandler(address='/dev/log')])


class ParseSize(argparse.Action):
    """
    Parse size args
    """

    def __init__(self, option_strings, dest, **kwargs):
        super().__init__(option_strings, dest, **kwargs)

    def __call__(self, parser, namespace, value, option_string=None):
        setattr(namespace, self.dest, human_to_bytes(value))


def extract_mtime(path):
    """
    Get mtime for purposes of sorting.
    If failed to determine, assume march 2038 (newest)
    """
    try:
        return getmtime(path)
    except OSError:
        pass
    return NEWEST_TS


def get_file_sizes(pool):
    """
    Given path, calculates its size.

    Things like block size are not accounted for,
    so result is what you get by calling `du` with --apparent-size arg.
    """
    size = {}
    for filepath in pool:
        try:
            size[filepath] = getsize(filepath)
        except OSError:
            pass
    return size


def get_pool(path, mask):
    """
    Returns a flat list of paths, including subdirs
    """
    pool = []
    for cur_dir, _, files in os.walk(path):
        for name in files:
            filepath = join(cur_dir, name)
            if islink(filepath):
                continue
            if mask.search(filepath):
                pool.append(filepath)
    return pool


def get_oldest_logs(paths, sorter=extract_mtime):
    """
    Returns a list of logs ordered by `sorter` function.
    Uses mtime by default.
    """
    return sorted(
        paths.keys(),
        key=sorter,
        reverse=False)


def unlink(path):
    LOG.debug('removing: %s', path)
    os.unlink(path)


def trim(files, remove_fun=unlink):
    """
    Removes a list of files.
    """
    for path in files:
        remove_fun(path)


def echo(message):
    """
    Used for dry runs
    """
    return lambda path: print('{0}: {1}'.format(message, path), file=sys.stderr)


def truncate(path):
    LOG.warning('truncate file: %s', path)
    with open(path, 'w') as f:
        f.truncate(0)


def percentage_to_bytes(used_percentage, pool_path):
    """
    Calculate exact pool size (in bytes) using given target disk usage
    E.g.: disk is 100G, used_percentage = 10, pool size will be 10G
    """
    dstats = shutil.disk_usage(pool_path)
    return dstats.total * used_percentage / 100


def human_to_bytes(human_size):
    """
    Converts 16K to 16384, 16M to 16777216 and so forth.
    """
    size, unit = float(human_size[:-1]), human_size[-1]
    units = {
        'b': 1,
        'k': 1024,
        'm': 1024 ** 2,
        'g': 1024 ** 3,
        't': 1024 ** 4,
    }
    try:
        return units[unit.lower()] * size
    except KeyError:
        raise ValueError('unknown unit: {}'.format(unit))


def resolve_pool_size(fixed_size, relative_size, pool_path):
    if fixed_size is not None:
        return fixed_size
    if relative_size is not None:
        return percentage_to_bytes(relative_size, pool_path)
    raise RuntimeError('Both fixed and relative sizes are None')


def parse_args():
    parser = argparse.ArgumentParser(description='Log size manager')

    def add_common(sub_parser):
        sub_parser.add_argument(
            '--pool',
            help='Where to look for files',
            required=True,
            type=str)
        sub_parser.add_argument(
            '--mask',
            help='Filter filenames by this mask',
            required=True,
            type=str)
        sub_parser.add_argument(
            '--dry-run',
            help='Do not do actual removal of files, just print to stderr',
            required=False,
            action='store_true',
            default=False)
        sub_parser.add_argument(
            '--usage-threshold',
            help='No action is performed if disk usage is below this threshold',
            required=False,
            default=0,
            metavar='0-100')

    actions = parser.add_subparsers()

    truncate_op = actions.add_parser('truncate-unrotatable', help='Truncate log file if it is too large to logrotate')
    truncate_op.set_defaults(func=truncate_unrotatable)
    add_common(truncate_op)

    # clear_op specific args
    clear_op = actions.add_parser('clear-old-logs', help='Remove old compressed logs')
    clear_op.set_defaults(func=clear_old_logs)
    add_common(clear_op)

    clear_op.add_argument(
        '--minfiles',
        help='Minimal number of files to keep',
        required=False,
        default=0,
        type=int,
    )
    sizeargs = clear_op.add_mutually_exclusive_group(required=True)
    sizeargs.add_argument(
        '--size',
        help='How much files to keep in bytes',
        metavar='X[m|g|t]',
        action=ParseSize,
        type=str)
    sizeargs.add_argument(
        '--max-disk-percentage',
        help='How much files to keep relative to the size of the FS, in percents',
        metavar='0-100',
        type=int)

    return parser.parse_args()


def clear_old_logs(args):
    mask = re.compile(args.mask)
    target_pool_size = resolve_pool_size(
        fixed_size=args.size,
        relative_size=args.max_disk_percentage,
        pool_path=args.pool)
    sizes = get_file_sizes(
        get_pool(args.pool, mask))
    # sizes: {'/var/log/aptitude.1,gz': 14882, ...}
    current_size = sum(sizes.values())

    delete_list = []
    # Oldest logs are first to go.
    logs_list = get_oldest_logs(sizes)

    max_deletions = max(len(logs_list) - args.minfiles, 0)
    for path in logs_list[:max_deletions]:
        if current_size <= target_pool_size:
            break
        delete_list.append(path)
        current_size -= sizes[path]

    trim(delete_list, remove_fun=echo('removing') if args.dry_run else unlink)


def truncate_unrotatable(args):
    mask = re.compile(args.mask)
    sizes = get_file_sizes(get_pool(args.pool, mask))

    available_space = shutil.disk_usage(args.pool).free

    truncate_list = []
    for log, size in sizes.items():
        if available_space < size * SIZE_SCALE_FACTOR:
            truncate_list.append(log)
            available_space += size
    trim(truncate_list, remove_fun=echo('truncating') if args.dry_run else truncate)


def check_preconditions(args):
    """ Check if preconditions are met for the run """

    usage = shutil.disk_usage(args.pool)

    # Calculate disk usage in percents. Actions are taken if above the threshold.
    # "Total - Free" is more pessimistic and reports a bigger number.
    # >>> usage = shutil.disk_usage('/')
    # >>> 100 - (usage.free / usage.total * 100)
    # 37.932296678375685
    # >>> usage.used / usage.total * 100
    # 32.807477024061136
    # >>>
    # $ df -h /
    # Filesystem      Size  Used Avail Use% Mounted on
    # /dev/nvme0n1p3   92G   30G   57G  35% /

    return 100 - (usage.free / usage.total * 100) > float(args.usage_threshold)


def main():
    args = parse_args()
    if not check_preconditions(args):
        return
    args.func(args)


if __name__ == '__main__':
    main()
