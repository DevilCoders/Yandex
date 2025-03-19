#!/usr/bin/env python3

import contextlib
import hashlib
import logging
import os
import subprocess
import sys
from tempfile import NamedTemporaryFile
from urllib.request import urlopen

__all__ = ['TestImage']

logger = logging.getLogger(__name__)


def runcmd(*args):
    logger.debug('{!r}'.format(args))
    subprocess.run(args, stdin=subprocess.DEVNULL, check=True)


def mk_overlay(img, backing):
    qemu_img = os.getenv('TEST_QEMU_IMG', 'qemu-img')

    backing_rel = os.path.relpath(os.path.abspath(backing),
                                  os.path.dirname(os.path.abspath(img)))
    runcmd(qemu_img, 'create', '-q', '-f', 'qcow2',
           '-b', backing_rel, '-F', 'qcow2', img)


class TestImage(object):
    url = 'https://storage.yandexcloud.net/yc-hypervisor-ci/fedora33.qcow2'
    csum_algo = 'md5'
    csum = '7c2378e66600c5757277745d5587e913'

    def __init__(self):
        self.cache_dir = os.getenv('TEST_CACHE_DIR', os.path.dirname(__file__))
        base_fname = 'base-{:.20}.qcow2'.format(self.csum)
        self.base_img = os.path.join(self.cache_dir, base_fname)

        if os.path.isfile(self.base_img):
            logger.debug('reusing existing base image "{}"'
                         .format(self.base_img))
        else:
            self._download()


    def _download(self):
        bs = 1 << 20
        rep_bs = 100
        dlcsum = hashlib.new(self.csum_algo)
        os.makedirs(self.cache_dir, exist_ok=True)
        with NamedTemporaryFile(dir=self.cache_dir, suffix='.qcow2') as tmpf:
            logger.debug('downloading "{}"'.format(self.url))
            with contextlib.closing(urlopen(self.url)) as ufp:
                size = 0
                while True:
                    block = ufp.read(bs)
                    if not block:
                        break

                    size += len(block)
                    if size % (bs * rep_bs) < bs:
                        logger.debug('downloading "{}": {}'.format(self.url, size))
                    tmpf.write(block)
                    dlcsum.update(block)

            tmpf.flush()
            assert(dlcsum.hexdigest() == self.csum)
            logger.debug('downloaded "{}" to "{}": {}'.
                         format(self.url, self.base_img, size))

            os.fchmod(tmpf.fileno(), 0o444)
            os.link(tmpf.name, self.base_img)


    def mkimg(self, fname):
        work_dir = os.path.dirname(fname)

        with NamedTemporaryFile(dir=work_dir, suffix='.qcow2') as tmpf:
            mk_overlay(tmpf.name, self.base_img)
            os.link(tmpf.name, fname)

        logger.info('set up test image "{}"'.format(fname))


def main():
    logging.basicConfig(level=logging.DEBUG)

    testimg = TestImage()

    if len(sys.argv) > 1:
        testimg.mkimg(sys.argv[1])


if __name__ == '__main__':
    main()
