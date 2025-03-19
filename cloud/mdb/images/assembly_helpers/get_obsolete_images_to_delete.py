#!/usr/bin/env python3

import argparse
import datetime
import json
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--branch', type=str, help='branch name')
    parser.add_argument('--images-to-keep', type=int, help='number of images to keep', default=3)
    args = parser.parse_args()

    images = json.loads(sys.stdin.read())
    image_id_by_creation_time = {}
    datetime_format = '%Y-%m-%dT%H:%M:%SZ'

    for image in images:
        if args.branch and args.branch != image.get('labels', {}).get('branch'):
            continue
        creation_time = datetime.datetime.strptime(image['created_at'], datetime_format)
        image_id_by_creation_time[image['id']] = creation_time

    images_sorted_by_creation_time = sorted(image_id_by_creation_time.items(), key=lambda item: item[1], reverse=True)
    for image_id, _ in images_sorted_by_creation_time[args.images_to_keep:]:
        print(image_id)


if __name__ == '__main__':
    main()
