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


class ImageNotFoundError(Exception):
    def __init__(self, message):
        super().__init__(message)


def runcmd(*args):
    logger.debug('%s', args)
    subprocess.run(args, stdin=subprocess.DEVNULL, check=True)


def mk_overlay(img, backing):
    qemu_img = os.getenv('TEST_QEMU_IMG', 'qemu-img')

    runcmd(qemu_img, 'create', '-q', '-f', 'qcow2',
           '-b', backing, '-F', 'qcow2', img)


class TestImage(object):
    FEDORA33_IMAGE_ID = '7c2378e66600c5757277745d5587e913'
    WINDOWS10_IMAGE_ID = 'ae8ac296b85d6be17c3eebe7f569647f'

    images = {FEDORA33_IMAGE_ID:
                'https://storage.yandexcloud.net/yc-hypervisor-ci/fedora33.qcow2',
            WINDOWS10_IMAGE_ID:
                'https://storage.yandexcloud.net/yc-hypervisor-ci/win10_x64_ci.qcow2'}
    csum_algo = 'md5'

    def _get_image_path(self, csum):
        base_fname = 'base-{:.20}.qcow2'.format(csum)
        return os.path.join(self.cache_dir, base_fname)

    def _get_image(self, csum):
        if csum in self.images:
            url = self.images[csum]
            base_img = self._get_image_path(csum)
            if os.path.isfile(base_img):
                logger.debug('reusing existing base image "%s"', base_img)
            else:
                self._download(url, csum, base_img)

            return base_img

        raise ImageNotFoundError("image with csum {} not found".format(csum))

    def __init__(self):
        self.cache_dir = os.getenv('TEST_CACHE_DIR', os.path.dirname(__file__))

    def _download(self, url, csum, base_img):
        bs = 1 << 20
        rep_bs = 100
        dlcsum = hashlib.new(self.csum_algo)
        os.makedirs(self.cache_dir, exist_ok=True)
        with NamedTemporaryFile(dir=self.cache_dir, suffix='.qcow2') as tmpf:
            logger.debug('downloading "%s"', url)
            with contextlib.closing(urlopen(url)) as ufp:
                size = 0
                while True:
                    block = ufp.read(bs)
                    if not block:
                        break

                    size += len(block)
                    if size % (bs * rep_bs) < bs:
                        logger.debug('downloading "%s": %d', url, size)
                    tmpf.write(block)
                    dlcsum.update(block)

            tmpf.flush()
            assert (dlcsum.hexdigest() == csum)
            logger.debug('downloaded "%s" to "%s": %s', url, base_img,
                         size)

            os.fchmod(tmpf.fileno(), 0o444)
            os.link(tmpf.name, base_img)

    def mkimg(self, fname, csum):
        work_dir = os.path.dirname(fname)

        with NamedTemporaryFile(dir=work_dir, suffix='.qcow2') as tmpf:
            mk_overlay(tmpf.name, self._get_image(csum))
            os.link(tmpf.name, fname)

        logger.info('set up test image "%s"', fname)

    @staticmethod
    def mk_file(name, size):
        fd = os.open(name, os.O_RDWR | os.O_CREAT, 0o644)
        os.ftruncate(fd, size)
        os.close(fd)


def main():
    logging.basicConfig(level=logging.DEBUG)

    testimg = TestImage()

    if len(sys.argv) > 1:
        testimg.mkimg(sys.argv[1], testimg.FEDORA33_IMAGE_ID)


if __name__ == '__main__':
    main()
