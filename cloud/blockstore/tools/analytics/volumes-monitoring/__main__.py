import argparse
import json
import logging
import signal
import threading
import time

from cloud.blockstore.public.sdk.python.client import CreateClient


data_logger = None
err_logger = None
shouldStop = False


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
        ch = logging.StreamHandler()
        ch.setLevel(logging.DEBUG)
        ch.setFormatter(formatter)
        logger.addHandler(ch)

    return logger


def sigterm(signum, frame):
    global shouldStop
    shouldStop = True


def make_response_handler(volume, counter):
    def response_handler(future):
        try:
            result = future.result()
        except Exception as e:
            err_logger.exception("Got exception from GRPC: {}".format(e))
        else:
            print(json.dumps(
                {'volume': volume,
                 'mixedblockscount': result['Stats'].MixedBlocksCount,
                 'mergedblockscount': result['Stats'].MergedBlocksCount}))
        counter.decrement()

    return response_handler


def run():
    global data_logger
    global err_logger

    signal.signal(signal.SIGINT, sigterm)
    signal.signal(signal.SIGQUIT, sigterm)
    signal.signal(signal.SIGTERM, sigterm)

    parser = argparse.ArgumentParser()
    parser.add_argument('--endpoint', help='server endpoint', default='localhost')
    parser.add_argument('--port', help='nbs grpc port', default=9766)
    parser.add_argument('--period', help='query period', default=0)
    parser.add_argument('--log', help='log file', default="")

    args = parser.parse_args()

    data_logger = setup_logger(
        'volume-monitoring',
        args.log,
        '%(asctime)s,%(message)s')

    err_logger = setup_logger(
        'volume-monitoring-error',
        'volume-monitoring-error.err' if args.log != "" else "",
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    client = CreateClient('{}:{}'.format(args.endpoint, args.port), log=logging)

    while True:
        try:
            volumes = client.list_volumes()
        except Exception as e:
            err_logger.exception('Got exception in nbs.list_volumes: {}'.format(str(e)))
        else:
            counter = Counter(len(volumes))
            for volume in volumes:
                future = client.stat_volume_async(disk_id=volume)
                future.add_done_callback(make_response_handler(volume, counter))
            counter.wait_zero()

        time.sleep(float(args.period))

        if args.period == 0 or (shouldStop is True):
            break


run()
