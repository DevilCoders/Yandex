import logging

import yatest.common as common

import subprocess


BINARY_PATH = common.binary_path("cloud/blockstore/support/CLOUDINC-1800/tools/copy_dev/copy_dev")
TOTAL_SIZE = 1024**3
BLOCK_SIZE = 4096
SAMPLE_SIZE = 100
BLOCKS_COUNT = TOTAL_SIZE / BLOCK_SIZE
SRC_FILE='src.bin'
DST_FILE='dst.bin'


def files_equal(path_0, path_1):
    file_0 = open(path_0, "rb")
    file_1 = open(path_1, "rb")

    while True:
        block_0 = file_0.read(BLOCK_SIZE)
        block_1 = file_1.read(BLOCK_SIZE)

        if block_0 != block_1:
            return False

        if not block_0:
            break

    return True


def copy_dev(src, dst):
    args = [BINARY_PATH] + [
        "--src-path", src,
        "--dst-path", dst,
        "--buffer-size", str(128*1024),
        "--verbose"
    ]

    process = subprocess.Popen(args)
    process.communicate()

    assert process.returncode == 0


def dd(src, dst):
    args = [
        'dd', f'if={src}', f'of={dst}',
        'bs=1M', f'count={ TOTAL_SIZE // 1024**2 }',
        'iflag=fullblock'
    ]

    logging.info(f"dd: {args}")

    process = subprocess.Popen(args)
    _, error = process.communicate()

    assert process.returncode == 0, error


def create_src_file():
    dd('/dev/urandom', SRC_FILE)

    return SRC_FILE


def create_dst_file():
    dd('/dev/zero', DST_FILE)

    return DST_FILE


def test_copy():
    src = create_src_file()
    dst = create_dst_file()

    assert not files_equal(src, dst)

    copy_dev(src, dst)

    assert files_equal(src, dst)
