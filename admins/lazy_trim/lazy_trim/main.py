#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
main module defines runner for lazy_trim
"""

import argparse
import ctypes
import fcntl
import logging
import math
import os
import sys
import time

import lockfile

DEFAULT_LOCK_FILE = '/var/run/lazy-trim'
DEFAULT_LOG_FILE = None
DEFAULT_BANDWIDTH = 1024  # 1GB/s
DEFAULT_VERBOSE = False
DEFAULT_MOUNT_POINT = '/data'
DEFAULT_TIME_FACTOR = 0.5
DEFAULT_TIMEOUT = 0

FITRIM = 0xc0185879


class Runner:
    """
    Runner-class for utility
    """

    # pylint: disable=too-few-public-methods
    def __init__(self,
                 mountpoint=DEFAULT_MOUNT_POINT,
                 bandwidth=DEFAULT_BANDWIDTH,
                 logfile=DEFAULT_LOG_FILE,
                 lock_file=DEFAULT_LOCK_FILE,
                 verbose=DEFAULT_VERBOSE,
                 timeout=DEFAULT_TIMEOUT):
        # pylint: disable=too-many-arguments
        self._mountpoint = mountpoint
        self._logfile = logfile
        self._verbose = verbose
        self._lockfile = lock_file
        self._setup_logging()
        self._trim = ThrottledTRIM(mountpoint, bandwidth, timeout)

    def start(self):
        """
        Run trim
        """
        lock = lockfile.LockFile(self._lockfile)
        try:
            with lock:
                logging.debug('Successfully acquired lockfile')
                self._trim.run()

        except lockfile.Error as exc:
            logging.error('Cannot acquire lock %s with exception: %s', self._lockfile, exc)
            sys.exit(1)

    def _setup_logging(self):
        """
        Configure logging according to config and setup class logger
        """
        log_level = 'INFO'
        if self._verbose:
            log_level = 'DEBUG'
        log_format = '%(asctime)s [%(levelname)s]: %(message)s'
        logging.basicConfig(
            level=getattr(logging, log_level, None),
            format=log_format,
            datefmt='%d/%m/%Y %H:%M:%S %Z',
            filename=self._logfile)
        self._logger = logging.getLogger('lazy-trim')


class FSTrimArgs(ctypes.Structure):
    """
    Structure for ioctl call
    """
    # pylint: disable=too-few-public-methods
    _fields_ = [
        ('start', ctypes.c_ulong),
        ('count', ctypes.c_ulong),
        ('min_len', ctypes.c_ulong),
    ]


class ThrottledTRIM:
    """
    Run throttled TRIM
    """

    # pylint: disable=too-few-public-methods
    def __init__(self, path, bandwidth, timeout, min_len=4096):
        self._path = path
        self._bandwidth = bandwidth
        self._min_len = min_len
        self._timeout = timeout

    def _trim(self, current_pos, count_size):
        # pylint: disable=attribute-defined-outside-init
        args = FSTrimArgs()
        args.start = current_pos
        args.count = count_size
        args.min_len = self._min_len
        handler = None
        return_code = -1
        trimmed = None
        try:
            try:
                handler = os.open(self._path, os.O_DIRECTORY)
                return_code = fcntl.ioctl(handler, FITRIM, args)
                trimmed = args.count
            except OSError as exception:
                return_code = exception.errno
                logging.exception(exception)
        finally:
            if handler is not None:
                os.close(handler)
        return (return_code, trimmed)

    def run(self):
        """
        Calculate throttling and run TRIM
        """
        # pylint: disable=too-many-locals
        step = int(self._bandwidth * 1024 * 1024 * DEFAULT_TIME_FACTOR)
        current_pos = 0
        res = os.statvfs(self._path)
        end_pos = res.f_bsize * res.f_blocks

        overall_penalties = 0
        overall_trimmed_steps = 0
        overall_trimmed_bytes = 0
        overall_trimmed_wait_time = 0.0

        overall_start_ts, end_ts = time.time(), time.time()
        while current_pos < end_pos:
            start_ts = time.time()
            if self._timeout != 0 and start_ts - overall_start_ts >= self._timeout:
                percent = (100.0 * current_pos / end_pos)
                logging.warning('Stop due overall timeout, checked {percent:.2f}%, {current}/{end} bytes'.format(
                    percent=percent, current=current_pos, end=end_pos))
                break
            _, trimmed = self._trim(current_pos, step)
            end_ts = time.time()
            current_pos = current_pos + step
            trim_time = end_ts - start_ts
            progress = 100.0 * current_pos / end_pos
            trimmed_mb = int(trimmed / 1024 / 1024)
            if trimmed:
                overall_trimmed_steps += 1
                overall_trimmed_bytes += trimmed
                overall_trimmed_wait_time += trim_time
            delay = 0
            if trim_time < DEFAULT_TIME_FACTOR:
                if trimmed:
                    delay = DEFAULT_TIME_FACTOR - trim_time
            else:
                # If trim_time is near the one second, this means one of two:
                # * a lot of trimmed blocks, and we should decrease the bandwidth of lazy_trim
                # * the disk system under heavy load, and we should also decrease influence of lazy_trim on I/O
                # So, add some penalty to sleep as a twice trim_time
                delay = min(math.ceil(trim_time * 2), 30)
                overall_penalties += 1
            logging.info('Step, progress={progress:.2f}%, current_position={pos}, trimmed={trimmed}MB, '
                         'time={tm:.3f}, delay={delay:.3f}s'.format(
                             pos=current_pos,
                             tm=(end_ts - start_ts),
                             trimmed=trimmed_mb,
                             progress=progress,
                             delay=delay))
            if delay:
                time.sleep(delay)
        logging.info('Mountpoint {point} has been trimmed {trimmed}MB, total time is {time:.3f}s, '
                     'wait time is {wait_time:.3f}s, trimmed steps {steps}, trimmed penalties {penalties}'.format(
                         point=self._path,
                         trimmed=int(overall_trimmed_bytes / 1024 / 1024),
                         time=(end_ts - overall_start_ts),
                         wait_time=overall_trimmed_wait_time,
                         steps=overall_trimmed_steps,
                         penalties=overall_penalties))


def main():
    """
    Entry point
    """
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument(
        '-b',
        '--bandwidth',
        type=int,
        default=DEFAULT_BANDWIDTH,
        help='Bandwidth for trimming in MB, default: {bw}'.format(bw=DEFAULT_BANDWIDTH))
    arg_parser.add_argument(
        '-t',
        '--timeout',
        type=int,
        default=DEFAULT_TIMEOUT,
        help='Timeout for trimming in seconds, default: {to} (without timeout)'.format(to=DEFAULT_TIMEOUT))
    arg_parser.add_argument(
        '-m',
        '--mountpoint',
        type=str,
        default=DEFAULT_MOUNT_POINT,
        help='Mountpoint to be trimmed, default: {mp}'.format(mp=DEFAULT_MOUNT_POINT))
    arg_parser.add_argument(
        '-l', '--logfile', type=str, default=DEFAULT_LOG_FILE, help='Log file, default: stdout/stderr')
    arg_parser.add_argument(
        '-x',
        '--lockfile',
        type=str,
        default=DEFAULT_LOCK_FILE,
        help='Lock file, default: {lock}'.format(lock=DEFAULT_LOCK_FILE))
    arg_parser.add_argument('-v', '--verbose', help='Verbose output', action='store_true')

    args = arg_parser.parse_args()

    runner = Runner(args.mountpoint, args.bandwidth, args.logfile, args.lockfile, args.verbose, args.timeout)
    runner.start()


if __name__ == '__main__':
    main()
