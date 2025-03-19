import os
import random
import select
import string
from typing import List
from unittest.mock import MagicMock, patch

import pytest

from agent.process_buffer import ProcessOutputBuffer

TEST_READ_CHUNK_SIZE = 1024  # 1 KiB
TEST_BUFFER_SIZE = 1024 * 3  # 3 KiB

testdata_bytes = bytes(
    "".join(map(lambda _: random.choice(string.ascii_letters + string.digits), range(TEST_READ_CHUNK_SIZE - 10))),
    encoding="utf-8"
)


def test_process_buffer_read():
    source = MagicMock()
    source.fileno = MagicMock()
    source.fileno.return_value = 11
    source.close = MagicMock()

    buffer = ProcessOutputBuffer(source)
    buffer.BUFFER_SIZE = TEST_BUFFER_SIZE
    buffer.READ_CHUNK_SIZE = TEST_READ_CHUNK_SIZE

    assert buffer.get_data() == b""
    assert buffer.total_read == 0

    # check read_once
    with patch.object(select, "select", new=patch_select_available()):
        with patch.object(source, "read", return_value=testdata_bytes) as read_func:
            buffer.read_once()
            read_func.assert_called_once_with(TEST_READ_CHUNK_SIZE)

    assert buffer.total_read == len(testdata_bytes)
    assert buffer.get_data() == testdata_bytes

    # check read in the same buffer
    with patch.object(select, "select", new=patch_select_available()):
        with patch.object(source, "read", return_value=testdata_bytes) as read_func:
            buffer.read_once()
            buffer.read_once()
            read_func.assert_called_with(TEST_READ_CHUNK_SIZE)

    expected_data = testdata_bytes * 3
    assert buffer.total_read == len(expected_data)
    assert buffer.get_data() == expected_data

    # check the size of the last chunk was calculated correctly
    expected_chunk_size = TEST_BUFFER_SIZE - len(expected_data)
    with patch.object(select, "select", new=patch_select_available()):
        with patch.object(source, "read", return_value=testdata_bytes[:expected_chunk_size]) as read_func:
            buffer.read_once()
            read_func.assert_called_once_with(expected_chunk_size)

    expected_data = (testdata_bytes * 4)[:TEST_BUFFER_SIZE]
    assert buffer.total_read == len(expected_data)
    assert buffer.total_read == len(buffer.get_data())
    assert buffer.get_data() == expected_data
    assert source.close.called


@pytest.mark.parametrize(
    "buffer_data,expected_data",
    [
        ([b""], b""),
        ([b"1" * 1024, b"1" * 1024], b"1" * 1024 * 2),
        ([b"1" * 1024 * 10], b"1" * 1024 * 10),
    ],
    ids=("empty-data", "small-data", "data-size-is-equal-limit"),
)
def test_process_buffer_data(buffer_data: List[bytes], expected_data: bytes):
    source = MagicMock()
    source.fileno = MagicMock()
    source.fileno.return_value = 11
    source.close = MagicMock()

    buffer = ProcessOutputBuffer(source)
    buffer._data = buffer_data
    assert buffer.get_data() == expected_data


def patch_select_available():
    def patched_select(rlist, wlist, xlist, *args, **kwargs):
        return rlist, wlist, xlist

    return patched_select
