import logging
import os
import random
import time

import yatest.common as common
import ydb.tests.library.common.yatest_common as yatest_common

from cloud.blockstore.config.server_pb2 import TServerAppConfig, TServerConfig, TKikimrServiceConfig
from cloud.blockstore.tests.python.lib.loadtest_env import LocalLoadTest
from cloud.blockstore.tests.python.lib.nonreplicated_setup import allow_disk_allocations, \
    setup_disk_registry_config_simple
from cloud.blockstore.tests.python.lib.test_base import thread_count, get_nbs_counters, \
    get_sensor_by_name

import subprocess


BINARY_PATH = common.binary_path("cloud/blockstore/client/blockstore-client")
WRITEREQ_FILE = "writereq.bin"
DATA_FILE = "/dev/urandom"
BLOCK_SIZE = 4096
BLOCKS_COUNT = 25000
SAMPLE_SIZE = 100
CHANGED_BLOCKS_COUNT = 1234

NRD_BLOCKS_COUNT = 1024**3 // BLOCK_SIZE


def files_equal(path_0, path_1, cb=None):
    file_0 = open(path_0, "rb")
    file_1 = open(path_1, "rb")

    block_no = 0
    while True:
        block_0 = file_0.read(BLOCK_SIZE)
        block_1 = file_1.read(BLOCK_SIZE)

        if block_0 != block_1:
            if cb is None or not cb(block_no, block_0, block_1):
                return False

        if not block_0:
            break

        block_no += 1

    return True


def compare_bitmaps(path_0, path_1):
    errors = []

    def cb(block_no, block_0, block_1):
        per_block = BLOCK_SIZE * 8
        offset = per_block * block_no

        for i in range(BLOCK_SIZE):
            byte_offset = offset + i * 8
            byte_0 = ord(block_0[i])
            byte_1 = ord(block_1[i])
            for j in range(8):
                bit_offset = byte_offset + j
                bit_0 = (byte_0 >> j) & 1
                bit_1 = (byte_1 >> j) & 1
                if bit_0 != bit_1:
                    errors.append("bit %s: %s -> %s" % (bit_offset, bit_0, bit_1))

    files_equal(path_0, path_1, cb)

    return errors


def setup(with_nrd=False):
    server = TServerAppConfig()
    server.ServerConfig.CopyFrom(TServerConfig())
    server.ServerConfig.ThreadsCount = thread_count()
    server.ServerConfig.StrictContractValidation = True
    server.KikimrServiceConfig.CopyFrom(TKikimrServiceConfig())

    env = LocalLoadTest(
        "",
        server_app_config=server,
        use_in_memory_pdisks=True,
        with_nrd=with_nrd)

    env.results_path = yatest_common.output_path() + "/results.txt"
    env.results_file = open(env.results_path, "w")

    def run(*args, **kwargs):
        args = [BINARY_PATH] + list(args) + [
            "--host", "localhost",
            "--port", str(env.nbs_port)]
        input = kwargs.get("input")
        if input is not None:
            input = (input + "\n").encode("utf8")

        process = subprocess.Popen(
            args,
            stdout=env.results_file,
            stdin=subprocess.PIPE
        )
        process.communicate(input=input)

        assert process.returncode == kwargs.get("code", 0)

    if with_nrd:
        allow_disk_allocations(env.nbs_port, BINARY_PATH)
        setup_disk_registry_config_simple(
            env.devices,
            env.nbs_port,
            BINARY_PATH)

        while True:
            sensors = get_nbs_counters(env.mon_port)['sensors']

            free_bytes = get_sensor_by_name(sensors, 'disk_registry', 'FreeBytes', -1)
            if free_bytes > 0:
                break
            logging.info('wait for free bytes ...')
            time.sleep(1)

    return env, run


def random_writes(run, block_count=BLOCKS_COUNT):
    sample = random.sample(range(block_count), SAMPLE_SIZE)
    sample.sort()

    for i in range(0, SAMPLE_SIZE, 2):
        start_index = sample[i]
        blocks_count = sample[i + 1] - start_index

        with open(DATA_FILE, "rb") as r:
            with open(WRITEREQ_FILE, "wb") as w:
                block = r.read(BLOCK_SIZE * blocks_count)
                w.write(block)

        run("writeblocks",
            "--disk-id", "volume-0",
            "--input", WRITEREQ_FILE,
            "--start-index", str(start_index))


def test_successive_remounts_and_writes():
    env, run = setup()

    with open(WRITEREQ_FILE, "w") as wr:
        wr.write(" " * 1024 * 16)

    run("createvolume",
        "--disk-id", "vol0",
        "--blocks-count", str(BLOCKS_COUNT))

    run("writeblocks",
        "--disk-id", "vol0",
        "--start-index", "0",
        "--input", WRITEREQ_FILE)

    run("writeblocks",
        "--disk-id", "vol0",
        "--start-index", "0",
        "--input", WRITEREQ_FILE)

    run("readblocks",
        "--disk-id", "vol0",
        "--start-index", "0",
        "--blocks-count", "32")

    ret = common.canonical_file(env.results_path)
    env.tear_down()
    return ret


def test_read_all_with_io_depth():
    env, run = setup()

    run("createvolume",
        "--disk-id", "volume-0",
        "--blocks-count", str(BLOCKS_COUNT))

    random_writes(run)

    run("readblocks",
        "--disk-id", "volume-0",
        "--read-all",
        "--output", "readblocks-0")

    run("readblocks",
        "--disk-id", "volume-0",
        "--read-all",
        "--io-depth", "8",
        "--output", "readblocks-1")

    assert files_equal("readblocks-0", "readblocks-1")
    env.tear_down()


def test_read_with_io_depth():
    env, run = setup()

    run("createvolume",
        "--disk-id", "volume-0",
        "--blocks-count", str(BLOCKS_COUNT))

    random_writes(run)

    run("readblocks",
        "--disk-id", "volume-0",
        "--start-index", "0",
        "--blocks-count", "2000",
        "--output", "readblocks-0")

    run("readblocks",
        "--disk-id", "volume-0",
        "--start-index", "0",
        "--blocks-count", "2000",
        "--io-depth", "8",
        "--output", "readblocks-1")

    assert files_equal("readblocks-0", "readblocks-1")
    env.tear_down()


def test_backup():
    env, run = setup()

    run("createvolume",
        "--disk-id", "volume-0",
        "--blocks-count", str(BLOCKS_COUNT),
        "--tablet-version", "2")

    random_writes(run)

    run("backupvolume",
        "--disk-id", "volume-0",
        "--backup-disk-id", "volume-1",
        "--checkpoint-id", "checkpoint-0",
        "--io-depth", "16",
        "--changed-blocks-count", str(CHANGED_BLOCKS_COUNT),
        "--verbose")

    run("getchangedblocks",
        "--disk-id", "volume-0",
        "--start-index", "0",
        "--blocks-count", str(BLOCKS_COUNT),
        "--output", "changedblocks-0")

    run("getchangedblocks",
        "--disk-id", "volume-1",
        "--start-index", "0",
        "--blocks-count", str(BLOCKS_COUNT),
        "--output", "changedblocks-1")

    """
    with open("changedblocks-0", "r+b") as f:
        f.seek(128)
        f.write(bytearray([0 for i in range(128)]))

    with open("changedblocks-1", "r+b") as f:
        f.seek(256)
        f.write(bytearray([0 for i in range(128)]))
    """

    errors = compare_bitmaps("changedblocks-0", "changedblocks-1")

    run("readblocks",
        "--disk-id", "volume-0",
        "--start-index", "0",
        "--blocks-count", str(BLOCKS_COUNT),
        "--output", "readblocks-0")

    run("readblocks",
        "--disk-id", "volume-1",
        "--start-index", "0",
        "--blocks-count", str(BLOCKS_COUNT),
        "--output", "readblocks-1")

    if not files_equal("readblocks-0", "readblocks-1"):
        errors.append("volume data not equal")

    # restorevolume not supported for HDD/SSD
    run("restorevolume",
        "--disk-id", "volume-0",
        "--backup-disk-id", "volume-0.bkp",
        "--io-depth", "16",
        "--verbose", code=1)

    env.tear_down()

    if len(errors):
        errlog = os.path.join(common.output_path(), "test_backup_errors.txt")
        with open(errlog, "w") as f:
            for error in errors:
                f.write("%s\n" % error)
        assert False


def test_backup_and_restore_nrd():
    env, run = setup(with_nrd=True)

    run("createvolume",
        "--disk-id", "volume-0",
        "--blocks-count", str(NRD_BLOCKS_COUNT),
        "--storage-media-kind", "nonreplicated")

    random_writes(run, NRD_BLOCKS_COUNT)

    run("backupvolume",
        "--disk-id", "volume-0",
        "--backup-disk-id", "volume-0.bkp",
        "--storage-media-kind", "hdd",
        "--io-depth", "16",
        "--verbose")

    run("readblocks",
        "--disk-id", "volume-0",
        "--start-index", "0",
        "--blocks-count", str(NRD_BLOCKS_COUNT),
        "--output", "readblocks-0")

    run("readblocks",
        "--disk-id", "volume-0.bkp",
        "--start-index", "0",
        "--blocks-count", str(NRD_BLOCKS_COUNT),
        "--output", "readblocks-1")

    assert files_equal("readblocks-0", "readblocks-1")

    # spoil disk content
    random_writes(run, NRD_BLOCKS_COUNT)

    run("readblocks",
        "--disk-id", "volume-0",
        "--start-index", "0",
        "--blocks-count", str(NRD_BLOCKS_COUNT),
        "--output", "readblocks-3")

    assert not files_equal("readblocks-0", "readblocks-3")

    run("restorevolume",
        "--disk-id", "volume-0",
        "--backup-disk-id", "volume-0.bkp",
        "--io-depth", "16",
        "--verbose")

    run("readblocks",
        "--disk-id", "volume-0",
        "--start-index", "0",
        "--blocks-count", str(NRD_BLOCKS_COUNT),
        "--output", "readblocks-4")

    assert files_equal("readblocks-0", "readblocks-4")

    env.tear_down()


def test_destroy_volume():
    env, run = setup()

    run("createvolume",
        "--disk-id", "vol0",
        "--blocks-count", str(BLOCKS_COUNT))

    run("createvolume",
        "--disk-id", "vol1",
        "--blocks-count", str(BLOCKS_COUNT))

    run("destroyvolume",
        "--disk-id", "vol0",
        input=None,
        code=1)

    run("listvolumes")

    run("destroyvolume",
        "--disk-id", "vol0",
        input="xxx",
        code=1)

    run("listvolumes")

    run("destroyvolume",
        "--disk-id", "vol0",
        input="vol0",
        code=0)

    run("listvolumes")

    ret = common.canonical_file(env.results_path)
    env.tear_down()
    return ret
