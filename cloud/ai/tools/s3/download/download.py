#!/usr/bin/python3
# coding: utf-8

import argparse
import boto3
import json
import os


def _parse_args():
    parser = argparse.ArgumentParser(
        description='Download files from s3',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        '-s3_urls',
        help='json with s3_urls'
    )
    parser.add_argument(
        '-bucket',
        help='s3 bucket name',
    )
    return parser.parse_args()


def main():
    run(**vars(_parse_args()))


def run(s3_urls, bucket):
    with open(s3_urls) as f:
        json_array = json.load(f)

    store_list = (obj['s3_url'] for obj in json_array)

    session = boto3.session.Session()
    s3 = session.client(
        service_name='s3',
        endpoint_url='https://storage.yandexcloud.net'
    )

    for url in store_list:
        get_object_response = s3.get_object(Bucket=bucket, Key=url)
        with open("files/" + os.path.basename(url), "wb") as f:
            f.write(get_object_response['Body'].read())
