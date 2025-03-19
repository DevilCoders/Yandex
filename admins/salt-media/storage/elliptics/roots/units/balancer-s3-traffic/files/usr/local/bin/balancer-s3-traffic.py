#!/usr/bin/pymds
import os
import argparse
import urllib2
import urllib
import re
import logging
import json
from time import time, sleep, ctime
from boto.s3.connection import S3Connection
from infra.yasm.yasmapi import GolovanRequest

STATUSES = {
    2: 'CRIT',
    1: 'WARN',
    0: 'OK',
}

def upload_to_s3(data):
    conn = S3Connection(
        aws_access_key_id="{{ pillar['yav']['s3-access-key-robot-storage-duty-prod'] }}",
        aws_secret_access_key="{{ pillar['yav']['s3-secret-key-robot-storage-duty-prod'] }}",
        host='s3.mds.yandex.net'
    )
    conn.auth_region_name = 'us-east-1'
    bucket = conn.get_bucket('mds-service')

    bucket.new_key('buckets_sorted_rps').set_contents_from_filename(data)


def setup_logging(log_file):
    _format = logging.Formatter("[%(asctime)s] [%(name)s] %(levelname)s: %(message)s")
    _handler = logging.FileHandler(log_file)
    _handler.setFormatter(_format)
    logging.getLogger().setLevel(logging.DEBUG)
    logging.getLogger().addHandler(_handler)

def get_request(signal, bucket='', status=False, warn=0.5, crit=1, period=3600, retries=10):
    host = 'CON'
    et = time()
    st = et
    try:
        for timestamp, values in GolovanRequest(host, period, st, et, signal, max_retry=retries, connect_timeout=600):
            for _, v in values.items():
                diff = int(time()) - timestamp
                v = v/diff if diff != 0 else v
                if status:
                    if v > crit:
                        status = 2
                    elif v > warn:
                        status = 1
                    else:
                        status = 0
                    return timestamp, status, v
                return timestamp, v
    except urllib2.HTTPError as e:
        if e.code == 429:
            sleep(2)
            logger.info('Too many requests, another try with {}'.format(bucket))
            for timestamp, values in GolovanRequest(host, period, st, et, signal, max_retry=retries, connect_timeout=600):
                for _, v in values.items():
                    diff = int(time()) - timestamp
                    v = v/diff if diff != 0 else v
                    if status:
                        if v > crit:
                            status = 2
                        elif v > warn:
                            status = 1
                        else:
                            status = 0
                        return timestamp, status, v
                    return timestamp, v

def get_top(path):
    buckets_sum = 0
    ns = dict()
    signals = [
        "conv(mul(mdsproxy_unistat-s3mds_nginx_port_80_bytes_received_dmmm,8),Gi)",
        "conv(mul(mdsproxy_unistat-s3mds_nginx_port_443_bytes_received_dmmm,8),Gi)",
    ]

    if not os.path.exists(path):
        print("1;Sorted buckets list is not ready yet")
        return

    signal = [
        "itype=mdsproxy;ctype=prestable,production:sum({}, {})".format(signals[0], signals[1])
    ]
    buckets_all = get_request(signal)[1]
    buckets_90 = 0.9 * buckets_all
    logger.info('Total Gib: {}'.format(buckets_all))
    with open(path, 'r') as f:
        data = json.load(f)
        data_ts = int(data.get('timestamp'))
    logger.info('Get file with timestamp {}({})'.format(data_ts, ctime(data_ts)))
    for line in data.get('buckets'):
        signal = [
            "itype=mdsproxy;ctype=prestable,production;prj={}:sum({}, {})".format(line, signals[0], signals[1])
        ]
        if buckets_sum <= buckets_90:
            timestamp, value = get_request(signal, line)
            buckets_sum += value
            if value > 0.002:
                ns[line] = value
                logger.debug('Bucket: {} appended to final list, total: {}, current sum: {}'.format(line, buckets_90, buckets_sum))
            logger.info('Bucket: {} with {} Gib. Metrics were collected at {} ({})'.format(line, value, timestamp, ctime(timestamp)))
        else:
            sort_buckets = sorted(ns, key=ns.get, reverse=True)
            status = 0 if len(sort_buckets) == 0 else 1
            print("{};Total Gib: {}. Top 90% buckets: {}".format(status, buckets_all, sort_buckets))
            return
    sort_buckets = sorted(ns, key=ns.get, reverse=True)
    print("1;Total Gib: {}. Top buckets: {}".format(buckets_all, sort_buckets))


def get_rps(path, warn, crit):
    ns = dict()
    signals = [
        "conv(mul(mdsproxy_unistat-s3mds_nginx_port_80_bytes_received_dmmm,8),Gi)",
        "conv(mul(mdsproxy_unistat-s3mds_nginx_port_443_bytes_received_dmmm,8),Gi)",
    ]
    for line in path:
        signal = [
            "itype=mdsproxy;ctype=prestable,production;prj={}:sum({}, {})".format(
                line, signals[0], signals[1]
            )
        ]
        timestamp, status, value = get_request(signal, line, True, warn, crit)
        ns[line] = (STATUSES[status], value)
        logger.info('Bucket: {} with {} Gibps. Metrics were collected at {} ({})'.format(line, value, timestamp, ctime(timestamp)))
        sleep(0.1)
    return ns


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--ignore', help='Buckets to ignore (regexp)', type=str)
    parser.add_argument('-w', '--warn', default=0.5, help='default: 0.5', type=int)
    parser.add_argument('-c', '--crit', default=1, help='default: 1', type=int)
    parser.add_argument(
        '-l',
        '--log_file',
        default='/var/log/s3/balancer-top-clients.log',
        help='log file, default: /var/log/s3/balancer-top-clients.log',
        type=str,
    )
    parser.add_argument(
        '-f', '--format', help='Output format', choices=['monrun', 'juggler', 'juggler-batch']
    )
    return parser.parse_args()


def main():
    d = dict()
    logger.debug('Start with arguments: {}'.format(args))

    curl = urllib.urlopen('http://s3.mds.yandex.net/mds-service/buckets_names')
    buckets_list = curl.read().decode('utf-8').split('\n')
    reg = re.compile("^(internal-dbaas.+|yandexcloud-dbaas.+|cloud-storage.+|buttload.+)")
    similar = list(filter(reg.match, buckets_list))
    buckets_list = [i for i in buckets_list if i not in similar]
    logger.info('Found {} buckets'.format(len(buckets_list)))

    if args.ignore:
        ignore_list = args.ignore.replace(',', '|')
        r = re.compile(ignore_list)
        ignore_list = list(filter(r.match, buckets_list))
        logger.info('Ignore list: {}'.format(ignore_list))
        buckets_list = [i for i in buckets_list if i not in ignore_list]
        logger.info('Number of buckets after ignore_list: {}'.format(len(buckets_list)))

    if args.format in ['juggler', 'juggler-batch']:
        buckets_rps = get_rps(buckets_list, args.warn, args.crit)
        events = [
            {
                "host": "{}".format(bucket),
                "service": "balancer-s3-traffic",
                "status": value[0],
                "description": "{} Gibps".format(value[1]),
            }
            for bucket, value in buckets_rps.items()
        ]
        if args.format == 'juggler':
            events = {'events': events}
        print(json.dumps(events))
        sort_buckets = sorted(buckets_rps, key=buckets_rps.get, reverse=True)
        d['timestamp'] = int(time())
        d['buckets'] = sort_buckets
        with open(path_sorted, 'w') as f:
            json.dump(d, f)
        upload_to_s3(path_sorted)
    else:
        try:
            buckets_list_sorted = urllib.urlopen('http://s3.mds.yandex.net/mds-service/buckets_sorted_rps')
            buckets_sorted = buckets_list_sorted.read().decode('utf-8')
            with open(path_s3, 'w') as f:
                f.write(buckets_sorted)
            logger.info('Get buckets list from {}'.format(path_s3))
            get_top(path_s3)
            os.remove(path_s3)
        except Exception as e:
            logger.warn("Can't get sorted buckets list from s3, reason: {}".format(e))
            logger.info('Get buckets list from {}'.format(path_sorted))
            get_top(path_sorted)
    return args.format

args = get_args()
setup_logging(log_file=args.log_file)
logger = logging.getLogger(__name__)
path_sorted = '/tmp/buckets-sorted.json'
path_s3 = '/tmp/buckets-sorted-s3.json'


if __name__ == '__main__':
    try:
        f = main()
        logger.info('End of script for {} format'.format(f))
    except Exception as e:
        logger.exception('Script failed: {}'.format(e))
