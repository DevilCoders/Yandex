#! /usr/bin/python

import argparse
import configparser
import os
import re
import sys
from datetime import datetime
from StringIO import StringIO

import boto


class Upload:
    chunk_size_megs = 250
    chunk_size = 1024 * 1024 * chunk_size_megs

    def __init__(self):
        conf = configparser.ConfigParser()
        conf.read(os.path.expanduser('~/.s3cfg'))

        path = self.parse_args()
        self.conn = self.connect_boto(conf)
        self.upload_to(*path)

    @staticmethod
    def parse_args():
        ap = argparse.ArgumentParser()
        ap.add_argument('path', help='The path to upload to, like s3://music/foo')
        args = ap.parse_args()
        path = args.path
        m = re.match(r'^s3://([^/]+)/(.+)$', path)
        if not m:
            raise RuntimeError('The path must be in the format s3://bucket/path')
        return m.groups()

    @staticmethod
    def connect_boto(conf):
        access_key = conf['default']['access_key']
        secret_key = conf['default']['secret_key']
        host = conf['default']['host_base']
        return boto.connect_s3(access_key, secret_key, host=host)

    def upload_to(self, bucket, filename):
        buck = self.conn.get_bucket(bucket)
        mp = buck.initiate_multipart_upload(filename)
        part_num = 1
        start_time = datetime.now()
        while True:
            data = sys.stdin.read(self.chunk_size)
            if data == '':
                break
            sio = StringIO(data)
            mp.upload_part_from_file(sio, part_num=part_num)

            now = datetime.now()
            secs = (now - start_time).seconds
            megs = part_num * self.chunk_size_megs
            speed = megs if secs == 0 else megs / secs
            print('[{}] uploaded part {} ({} megs done, {} mbps avg)'.format(now.ctime(), part_num, megs, speed))
            sys.stdout.flush()
            part_num += 1
        mp.complete_upload()


if __name__ == '__main__':
    Upload()
