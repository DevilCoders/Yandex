from __future__ import print_function

import threading
import random
import fallocate
import mmap
import os
import contextlib
import time
import sys
import directio
import argparse

import six
from six.moves import queue as Queue

from library.python.bstr.common import gen_entropy

if six.PY3:
    long = int


def bind(f, *args, **kwargs):
    return lambda: f(*args, **kwargs)


@contextlib.contextmanager
def open_mmap(*args, **kwargs):
    mm = mmap.mmap(*args, **kwargs)
    yield mm
    mm.close()


@contextlib.contextmanager
def open_fd(*args, **kwargs):
    fd = os.open(*args, **kwargs)
    yield fd
    os.close(fd)


class MMapEngine(object):
    def write(self, fname, off, data):
        with open_fd(fname, os.O_RDWR) as fd:
            with open_mmap(fd, len(data), mmap.MAP_SHARED, mmap.PROT_WRITE, 0, off) as mm:
                mm.write(data)

    def read(self, fname, off, l):
        with open_fd(fname, os.O_RDONLY) as fd:
            with open_mmap(fd, l, mmap.MAP_SHARED, mmap.PROT_READ, 0, off) as mm:
                return mm.read(l)


class DirectEngine(object):
    def write(self, fname, off, data):
        with open_fd(fname, os.O_RDWR | os.O_DIRECT) as fd:
            os.lseek(fd, off, os.SEEK_SET)
            directio.write(fd, data)

    def read(self, fname, off, l):
        with open_fd(fname, os.O_RDONLY | os.O_DIRECT) as fd:
            os.lseek(fd, off, os.SEEK_SET)

            return directio.read(fd, l)


class PlainEngine(object):
    def write(self, fname, off, data):
        with open_fd(fname, os.O_RDWR) as fd:
            os.lseek(fd, off, os.SEEK_SET)
            os.write(fd, data)

    def read(self, fname, off, l):
        with open_fd(fname, os.O_RDONLY) as fd:
            os.lseek(fd, off, os.SEEK_SET)

            return os.read(fd, l)


def main(args):
    parser = argparse.ArgumentParser()

    parser.add_argument('--path')
    parser.add_argument('--blocks')
    parser.add_argument('--engine')

    args = parser.parse_args(args)

    fname = args.path
    blocks = long(args.blocks)
    bsize = 4 * 1024 * 1024
    ll = threading.Lock()

    engines = {
        'mmap': MMapEngine,
        'direct': DirectEngine,
        'plain': PlainEngine,
    }

    fs = engines[args.engine]()

    def falloc(s):
        with ll:
            print('falloc', s, file=sys.stderr)

        with open(fname, 'w') as f:
            fallocate.fallocate(f, 0, s)

    def readf(f, t):
        bb = fs.read(fname, f, t - f)
        lr = len(bb)
        bc = bb.count('b')

        with ll:
            print('read', f, lr, bc, file=sys.stderr)

    def writef(f, t):
        l = t - f

        fs.write(fname, f, str(gen_entropy(l)))

        with ll:
            print('write', f, l, file=sys.stderr)

    def stopf():
        raise StopIteration()

    falloc(blocks * bsize)

    in_q = Queue.Queue()

    def thr_func():
        while True:
            try:
                in_q.get()()
            except StopIteration:
                return

            time.sleep(0.08 * random.random())

    thrs = []

    for i in range(0, 2):
        thrs.append(threading.Thread(target=thr_func))

    for t in thrs:
        t.start()

    lst = list(range(0, blocks))
    in_use = []

    random.shuffle(lst)

    for v in lst:
        in_q.put(bind(writef, v * bsize, (v + 1) * bsize))
        in_use.append(v)

        if 1:
            if random.random() < 0.3:
                rv = random.choice(in_use)

                in_q.put(bind(readf, rv * bsize, (rv + 1) * bsize))

    for t in thrs:
        in_q.put(stopf)

    for t in thrs:
        t.join()
