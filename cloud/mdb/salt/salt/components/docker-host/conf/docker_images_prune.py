#!/usr/bin/env python3
"""
Remove unused docker images
"""

import argparse
import datetime
import logging
import time

import docker
from dateutil import parser

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')


def should_remove(image, now, tag_time):
    """
    Check if we should remove this image
    """
    try:
        if not image.attrs['RepoTags'] or image.labels.get('image-cleanup', 'true') != 'false':
            image_tag_time = parser.parse(image.attrs['Metadata']['LastTagTime'])
            return (now - image_tag_time).total_seconds() > tag_time
    except Exception as exc:
        logging.exception(exc)
    return False


def prune_images(tag_time, pause_time):
    """
    Remove unused images with pause between api calls
    """
    now = datetime.datetime.now().astimezone()
    api = docker.from_env()
    for image in api.images.list():
        if should_remove(image, now, tag_time):
            try:
                api.images.remove(image.id)
                logging.info('Removed image %s', image.id)
            except Exception as exc:
                logging.exception(exc)
            time.sleep(pause_time)


def _main():
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument('-t', '--tag-time', type=int, help='Clean images tagged more than N hours', default=4)
    args_parser.add_argument('-p', '--pause-time', type=int, help='Sleep N seconds between images removals', default=2)
    args = args_parser.parse_args()

    prune_images(args.tag_time * 3600, args.pause_time)


if __name__ == '__main__':
    _main()
