#!/usr/bin/env python3
"""
Get image from s3 and import to docker
"""

import fcntl
import getpass
import logging
import os
import random
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from multiprocessing import cpu_count

import boto
from dateutil import parser as dt_parser
from humanfriendly import format_timespan
from retrying import retry

from .docker import DOCKER_API
from .utils import env_stage


def should_download(image_name, last_modified):
    """
    Check if local image is older than last_modified
    """
    try:
        image = DOCKER_API.images.get('{repo}:latest'.format(repo=image_name))
        # Images less than 1MB are definitely broken
        if image.attrs['Size'] < 1048576:
            return True
        create_date = dt_parser.parse(image.attrs['Created'])
        s3_date = dt_parser.parse(last_modified)
        return s3_date > create_date
    except Exception:
        return True


@retry(wait_random_min=30000, wait_random_max=120000, stop_max_attempt_number=5)
def get_image(config, image_name):
    """
    Get image from S3 if local copy is stale
    """
    log = logging.getLogger(__name__)
    s3_client = boto.connect_s3(host=config['host'])
    bucket = s3_client.get_bucket(config['bucket'])
    image_path = config['images'][image_name]['path']
    images = bucket.list(image_path)
    try:
        newest = sorted(images, key=lambda x: x.last_modified)[-1]
    except IndexError:
        raise RuntimeError("There are no '{}' images in '{}' bucket".format(image_path, config['bucket']))

    with tempfile.TemporaryDirectory() as tmpdir:
        if should_download(image_name, newest.last_modified):
            log.info('getting template %s from S3.', image_name)
            start = time.time()
            image_file = os.path.join(tmpdir, '{name}.tar.gz'.format(name=image_name))
            newest.get_contents_to_filename(image_file)
            log.info('download complete in %s', format_timespan(time.time() - start))
            start = time.time()
            DOCKER_API.api.import_image_from_file(
                image_file, repository=image_name, tag='latest', changes=['LABEL image-cleanup=false'])
            os.unlink(image_file)
            log.info('import complete in %s', format_timespan(time.time() - start))


def get_image_with_lock(conf, image):
    """
    Call get image with file lock
    """
    lock_path = os.path.join('/tmp', '{user}-{image}.lock'.format(user=getpass.getuser(), image=image))
    with open(lock_path, 'w') as lock_file:
        fcntl.flock(lock_file, fcntl.LOCK_EX)
        get_image(conf['s3-templates'], image)


@env_stage('create', fail=True)
def get_images(*, conf, **_):
    """
    Update all local images from S3
    """
    images = list(conf['s3-templates']['images'])
    random.shuffle(images)
    with ThreadPoolExecutor(max_workers=cpu_count(), thread_name_prefix='s3-image-get-') as get_pool:
        futures = []
        for image in images:
            futures.append(get_pool.submit(get_image_with_lock, conf, image))

        for future in futures:
            future.result()
