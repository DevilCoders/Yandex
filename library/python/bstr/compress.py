import os
from random import random
from subprocess import check_call


def safe_compress(method, infile, outfile, threads=0, level=4):
    tmpfile = outfile + '.' + str(random())
    try:
        compress(method, infile, tmpfile, threads, level)
        os.rename(tmpfile, outfile)
    except Exception as e:
        try:
            os.unlink(tmpfile)
        except OSError:
            pass
        raise e


def safe_decompress(method, infile, outfile, threads=0):
    tmpfile = outfile + '.' + str(random())
    try:
        decompress(method, infile, tmpfile, threads=0)
        os.rename(tmpfile, outfile)
    except Exception as e:
        try:
            os.unlink(tmpfile)
        except OSError:
            pass
        raise e


def compress(method, infile, outfile, threads=0, level=4):
    if method == 'zstd':
        return compress_zstd(infile, outfile, threads, level)
    raise Exception('unknown compression method')


def decompress(method, infile, outfile, threads=0):
    if method == 'zstd':
        return decompress_zstd(infile, outfile, threads)
    raise Exception('unknown compression method')


def compress_zstd(infile, outfile, threads=0, level=4):
    cmd = ['/usr/bin/env', 'zstd', '-q', '-T{threads}'.format(threads=threads), '-{level}'.format(level=level), '-o', outfile, infile]
    check_call(cmd)


def decompress_zstd(infile, outfile, threads=0):
    cmd = ['/usr/bin/env', 'zstd', '-q', '-T{threads}'.format(threads=threads), '-d', '-o', outfile, infile]
    check_call(cmd)
