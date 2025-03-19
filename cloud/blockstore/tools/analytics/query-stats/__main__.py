import argparse
import json
import logging
import signal
import sys
import threading
import time

import cloud.blockstore.tools.nbsapi as nbs

from cloud.blockstore.public.sdk.python.client import CreateClient

data_logger = None
err_logger = None
shouldStop = False
output = []


class Counter:

    def __init__(self, count):
        self.counter = count
        self.event = threading.Event()

    def decrement(self):
        self.counter -= 1
        if self.counter == 0:
            self.event.set()

    def wait_zero(self):
        self.event.wait()


def setup_logger(name, filename, format):
    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)

    formatter = logging.Formatter(fmt=format, datefmt='%m/%d/%Y %I:%M:%S')

    if filename != "":
        fh = logging.FileHandler(filename)
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(formatter)
        logger.addHandler(fh)
    else:
        ch = logging.StreamHandler(sys.stdout)
        ch.setLevel(logging.DEBUG)
        ch.setFormatter(formatter)
        logger.addHandler(ch)

    return logger


def sigterm(signum, frame):
    global shouldStop
    shouldStop = True


def make_response_handler(volume, disk_info, counter):
    def response_handler(future):
        global output
        try:
            result = future.result()
        except Exception as e:
            err_logger.exception("Got exception from GRPC: {}".format(e))
        else:
            data = disk_info if disk_info is not None else {}
            data.update({
                'DiskId': result['Volume'].DiskId,
                'BlocksCount': result['Volume'].BlocksCount,
                'BlockSize': result['Volume'].BlockSize,
                'MediaKind': result['Volume'].StorageMediaKind,
                'FolderId': result['Volume'].FolderId,
                'CloudId': result['Volume'].CloudId,
                'MixedBlocksCount': result['Stats'].MixedBlocksCount,
                'MergedBlocksCount': result['Stats'].MergedBlocksCount,
                'FreshBlocksCount': result['Stats'].FreshBlocksCount,
            })
            output.append(data)
        counter.decrement()

    return response_handler


def resolve_tablets(client, volume):
    try:
        description = nbs.describe_volume(client, volume)
    except Exception as e:
        err_logger.exception('Got exception in nbs.describe_volume: {}'.format(str(e)))
    else:
        result = {'VolumeTabletId': description['VolumeTabletId']}
        for i in range(len(description['Partitions'])):
            result['Partition{}'.format(i)] = description['Partitions'][i]['TabletId']
        return result
    return {}


def run():
    global data_logger
    global err_logger

    signal.signal(signal.SIGINT, sigterm)
    signal.signal(signal.SIGQUIT, sigterm)
    signal.signal(signal.SIGTERM, sigterm)

    parser = argparse.ArgumentParser()
    parser.add_argument('--endpoint', help='server endpoint', default='localhost')
    parser.add_argument('--grpc-port', help='nbs grpc port', default=9766)
    parser.add_argument('--period', help='query period', default=0)
    parser.add_argument('--log', help='log file', default="")
    parser.add_argument('--resolve', help='resolve tablet ids', default=False, action="store_true")

    args = parser.parse_args()

    data_logger = setup_logger(
        'volume-monitoring',
        args.log,
        '%(message)s')

    err_logger = setup_logger(
        'volume-monitoring-error',
        "",
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    client = CreateClient('{}:{}'.format(args.endpoint, args.grpc_port), log=logging)

    while True:
        try:
            volumes = client.list_volumes()
        except Exception as e:
            err_logger.exception('Got exception in nbs.list_volumes: {}'.format(str(e)))
        else:
            volumes_count = Counter(len(volumes))
            for volume in volumes:
                disk_info = {}
                if args.resolve:
                    disk_info = resolve_tablets(client, volume)
                future = client.stat_volume_async(disk_id=volume)
                future.add_done_callback(
                    make_response_handler(volume, disk_info, volumes_count))
            volumes_count.wait_zero()

        data_logger.info(json.dumps(output))

        time.sleep(float(args.period))

        if args.period == 0 or (shouldStop is True):
            break


run()
