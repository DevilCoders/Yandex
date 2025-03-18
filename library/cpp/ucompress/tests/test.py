import glob
import pytest
import random
import string
import subprocess

import yatest.common as yac


uc_bin_path = yac.binary_path('tools/uc/uc')
tool_bin_path = yac.binary_path('library/cpp/ucompress/tests/tool/tool')

data = {
    '2k': b'a' * (1 << 10) + b'z' * (1 << 10),
    'rnd': ''.join(random.choices(string.printable, k=128 << 20)).encode(),
    '2a': b'aa',
    '1a': b'a',
    '0': b'',
}


@pytest.mark.parametrize('data_key', list(data.keys()))
def test_interop(data_key):
    file_orig = yac.test_output_path('file.orig')
    with open(file_orig, 'wb') as fp:
        fp.write(data[data_key])

    compress_args = ['-c', '-C', 'zstd_1']
    file_compressed_uc = yac.test_output_path('file.comp_uc')
    subprocess.run([uc_bin_path] + compress_args + ['-f', file_orig, '-t', file_compressed_uc], check=True)
    file_compressed_tool = yac.test_output_path('file.comp_tool')
    subprocess.run([tool_bin_path] + compress_args + ['-f', file_orig, '-t', file_compressed_tool], check=True)

    subprocess.run([uc_bin_path, '-d', '-f', file_compressed_uc, '-t', file_compressed_uc + '.decomp_uc'], check=True)
    subprocess.run([uc_bin_path, '-d', '-f', file_compressed_tool, '-t', file_compressed_tool + '.decomp_uc'], check=True)
    subprocess.run([tool_bin_path, '-d', '-f', file_compressed_uc, '-t', file_compressed_uc + '.decomp_tool'], check=True)
    subprocess.run([tool_bin_path, '-d', '-f', file_compressed_tool, '-t', file_compressed_tool + '.decomp_tool'], check=True)

    for decomp_file in glob.glob(yac.test_output_path() + '/.*decomp_*'):
        with open(decomp_file, 'rb') as fp:
            assert fp.read() == data[data_key], decomp_file
