import itertools
import json
import logging
import os
import subprocess
import random

import yatest.common as common

logger = logging.getLogger(__name__)

KB = 1024
MB = 1024*1024


def _print_size(size):
    if size < KB:
        return str(size)
    elif size < MB:
        return "{}K".format(int(size/KB))
    else:
        return "{}M".format(int(size/MB))


class TestCase:
    def __init__(self, scenario, size, request_size, iodepth, sync, duration, compress_percentage, verify):
        self._scenario = scenario
        self._size = size
        self._request_size = request_size
        self._iodepth = iodepth
        self._sync = sync
        self._duration = duration
        self._compress_percentage = compress_percentage
        self._verify = verify

    @property
    def scenario(self):
        return self._scenario

    @property
    def size(self):
        return self._size

    @property
    def request_size(self):
        return self._request_size

    @property
    def iodepth(self):
        return self._iodepth

    @property
    def sync(self):
        return self._sync

    @property
    def duration(self):
        return self._duration

    @property
    def compress_percentage(self):
        return self._compress_percentage

    @property
    def name(self):
        parts = [
            self.scenario,
            _print_size(self.request_size),
            str(self.iodepth)
        ]
        if self.sync:
            parts += ["sync"]
        return "_".join(parts)

    @property
    def verify(self):
        return self._verify


def _generate_tests(size, duration, sync, scenarios, sizes, iodepths, compress_percentage, verify):
    return [
        TestCase(scenario, size, request_size, iodepth, sync, duration, compress_percentage, verify)
        for scenario, request_size, iodepth in itertools.product(scenarios, sizes, iodepths)
    ]


def generate_tests(size=100*MB, duration=60, sync=False, scenarios=['randread', 'randwrite', 'randrw'],
                   sizes=[4*KB, 4*MB], iodepths=[1, 32], compress_percentage=90, verify=True):
    return {
        test.name: test
        for test in _generate_tests(size, duration, sync, scenarios, sizes, iodepths, compress_percentage, verify)
    }


def _get_fio_bin():
    fio_bin = common.build_path(
        "cloud/storage/core/tools/testing/fio/bin/fio")
    if not os.path.exists(fio_bin):
        raise Exception("cannot find fio binary at path " + fio_bin)
    return fio_bin


def get_file_name(mount_dir, test_name):
    if not os.path.exists(mount_dir):
        raise Exception("invalid path " + mount_dir)
    return "{}/{}.dat".format(mount_dir, test_name)


def _get_fio_cmd(fio_bin, file_name, test):
    cmd = [
        fio_bin,
        "--name", test.name,
        "--filename", file_name,
        "--rw", str(test.scenario),
        "--size", str(test.size),
        "--bs", str(test.request_size),
        "--buffer_compress_percentage", str(test.compress_percentage),
        "--runtime", str(test.duration),
        "--time_based",
        "--output-format", "json",
    ]
    if test.verify and 'read' not in test.scenario:
        cmd += [
            "--verify", "md5",
            "--verify_backlog", "8192",  # 32MiB
            # "--verify_fatal", "1",
            "--serialize_overlap", "1",
            "--verify_dump", "1",

        ]
    if test.sync:
        cmd += [
            "--buffered", "1",
            "--ioengine", "sync",
            "--numjobs", str(test.iodepth),
        ]
    else:
        cmd += [
            "--direct", "1",
            "--ioengine", "libaio",
            "--iodepth", str(test.iodepth),
        ]
    return cmd


def _lay_out_file(file_name, size):
    bs = 1 << 20
    buf = random.randbytes(bs)

    # do not touch pre-existing files (e.g. device nodes)
    try:
        fd = os.open(file_name, os.O_RDWR | os.O_CREAT | os.O_EXCL, 0o644)
    except OSError:
        return

    try:
        for pos in range(0, size, bs):
            left = size - pos
            os.write(fd, buf[:left])
    finally:
        os.close(fd)


def run_test(file_name, test):
    fio_bin = _get_fio_bin()

    # fio lays out the test file using the job blocksize, which may exhaust the
    # run time limit, so do it ourselves using bigger blocksize
    if test.scenario.endswith("read") or test.scenario.endswith("rw"):
        _lay_out_file(file_name, test.size)

    cmd = _get_fio_cmd(fio_bin, file_name, test)
    logger.info("execute " + " ".join(cmd))

    ex = common.execute(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT)

    results = json.loads(ex.stdout)

    # TODO: canonize something useful
    errors = 0
    for job in results["jobs"]:
        errors += int(job["error"])
    return "errors: " + str(errors)
