import argparse
import concurrent.futures
import json
import logging
import os
import progressbar
import re
import requests
import threading
import time

import cloud.blockstore.tools.nbsapi as nbs

from cloud.blockstore.public.sdk.python.client import CreateClient, ClientCredentials


UPTIME_PATTERN = re.compile(r"Uptime: (\d+)")
LOG_FORMAT = '%(asctime)s - %(levelname)s - %(funcName)s - %(message)s'


def grep(text, pattern):
    m = pattern.search(text)
    if m is None:
        return None
    return m


class ConcurrentPersistentCompletionSet(object):

    def __init__(self, file):
        self._data = set()
        self._lock = threading.Lock()
        self._f = None

        if file is not None:
            with open(file, 'r') as f:
                self._data = set(f.read().splitlines())
            self._f = open(file, 'a')

    def __del__(self):
        if self._f is not None:
            self._f.close()

    def add(self, element):
        with self._lock:
            if element in self._data:
                return False
            self._data.add(element)
            if self._f:
                self._f.write(element)
                self._f.write('\n')
                self._f.flush()
                os.fsync(self._f.fileno())
            return True


class MonitoringClient(object):

    def __init__(self, endpoint, log=logging.getLogger()):
        self._endpoint = endpoint
        self._log = log

    def compact_localdb(self, id):
        url = 'http://{}/tablets/executorInternals?TabletID={}&force_compaction={}'
        for i in range(1, 12):  # 11 = tables count
            try:
                response = requests.get(url.format(self._endpoint, id, i))
                response.raise_for_status()
            except Exception as e:
                raise Exception('HTTP GET request failed for tablet [id={}]: {}'.format(id, e))

        time.sleep(60)

    def kill_tablet(self, id):
        response = requests.get('http://{}/tablets/app?KillTabletID={}'.format(self._endpoint, id))
        response.raise_for_status()
        self._log.info('Killed tablet [id={}]'.format(id))

    def get_tablet_uptime(self, id):
        response = requests.get('http://{}/tablets?TabletID={}'.format(self._endpoint, id))
        response.raise_for_status()
        result = grep(response.text, UPTIME_PATTERN)
        if result is not None:
            return int(result.group(1))
        else:
            raise Exception("Cannot read uptime (tablet is possibly dead)")


class Compacter(object):

    def __init__(self,
                 grpc_client,
                 mon_client,
                 processed_tablets,
                 compact_target='blobs',
                 compact_action='compact',
                 aux_params='',
                 kill_tablets=False,
                 logger=logging.getLogger()):
        self._grpc_client = grpc_client
        self._mon_client = mon_client
        self._processed_tablets = processed_tablets
        self._compact_target = compact_target
        self._compact_action = compact_action
        self._aux_params = aux_params
        self._kill_tablets = kill_tablets
        self._log = logger

    def compact_volume(self, id):
        description = nbs.describe_volume(self._grpc_client, id)
        if description is None:
            self._log.info('Disk %s not found' % id)
            return True

        self._grpc_client.stat_volume(disk_id=id)

        blocks = 0
        vparts = description['VolumeConfig'].get('Partitions')
        parts = description.get('Partitions')

        if parts is None or vparts is None:
            self._log.error('Disk %s has bad config - no Partitions' % id)
            return False

        for i in range(len(vparts)):
            blocks += int(vparts[i]['BlockCount'])

            if self._compact_target == 'localdb':
                if not self.compact_tablet_localdb(parts[i]['TabletId']):
                    return False

        success = True

        if self._compact_target == 'blobs':
            j = nbs.compact_disk(self._grpc_client, id, 0, blocks)
            error = j.get('Error')
            if error is not None:
                self._log.error('Disk %s compaction failed: %s' % json.dumps(error))
                success = False
            else:
                op_id = j['OperationId']
                while True:
                    jj = nbs.get_compaction_status(self._grpc_client, id, op_id)
                    if jj.get('IsCompleted'):
                        break

                    error = j.get('Error')
                    if error is not None:
                        self._log.error('Disk %s compaction failed: %s' % json.dumps(error))
                        success = False
                        break

                    self._log.info('Disk %s compaction progress: %s / %s' % (
                        id, jj.get('Progress', 0), jj['Total']))
                    time.sleep(10)

        if not success:
            return False

        if self._kill_tablets:
            self._mon_client.kill_tablet(description['VolumeTabletId'])

        return True

    def compact_tablet_localdb(self, id):
        if not self._processed_tablets.add(id):
            return False

        try:
            uptime_old = self._mon_client.get_tablet_uptime(id)
        except Exception as e:
            self._log.warn(str(e))
            uptime_old = 0
        self._mon_client.compact_localdb(id)
        try:
            uptime_new = self._mon_client.get_tablet_uptime(id)
        except Exception as e:
            self._log.warn(str(e))
            uptime_new = 0

        if uptime_new < uptime_old:
            self._log.debug('Uptime has decreased [id={}]: {}->{}'.format(id, uptime_old, uptime_new))

        return True


def get_logger(log_level, log_file):
    logger = logging.getLogger()
    logger.setLevel(getattr(logging, log_level))
    stream_handler = logging.StreamHandler()
    stream_handler.setLevel(log_level)
    stream_handler.setFormatter(logging.Formatter(LOG_FORMAT))
    logger.addHandler(stream_handler)

    if log_file is not None:
        file_handler = logging.FileHandler(filename=log_file)
        file_handler.setLevel(log_level)
        file_handler.setFormatter(logging.Formatter(LOG_FORMAT))
        logger.addHandler(file_handler)

    return logger


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--monhost', default='localhost')
    parser.add_argument('--nbshost', default='localhost')
    parser.add_argument('--mon-port', help='NBS monitoring port', type=int, default=8766)
    parser.add_argument('--grpc-port', help='NBS gRPC port', type=int, default=9768)
    parser.add_argument('--iam-token-file', help='iam token path', type=str)

    parser.add_argument('--volume', help='Volume id', action='append', type=str)
    parser.add_argument('--volumes-file', help='File with volumes ids')
    parser.add_argument('--tablet', help='Tablet id', action='append', type=str)
    parser.add_argument('--tablets-file', help='File with tablets ids')
    parser.add_argument('--completion-file', help='Saves completed tablets into file')

    parser.add_argument('--compact-target', help='Compact blobs or local db', default='blobs',
                        choices=['blobs', 'localdb'])
    parser.add_argument('--compact-action', help='Compaction type', default='compact',
                        choices=['compact', 'rebuildMetadata'])
    parser.add_argument('--aux-params', help='Viewer action aux cgi param string', default='')
    parser.add_argument('--kill-tablets', help='Kill tablets after compaction', action='store_true')

    parser.add_argument('--batch', help='Number of parallel compactions', type=int, default=1)
    parser.add_argument('--log-level', '-l', default='ERROR', choices=['DEBUG', 'INFO', 'WARNING', 'ERROR'])
    parser.add_argument('--log-file', help='Enables logging in file')
    return parser.parse_args()


def main():
    args = parse_args()

    logger = get_logger(args.log_level, args.log_file)

    iam_token = None
    if args.iam_token_file is not None:
        with open(args.iam_token_file) as f:
            iam_token = f.read().rstrip().decode()

    grpc_client = CreateClient(
        endpoint='{}:{}'.format(args.nbshost, args.grpc_port),
        credentials=ClientCredentials(auth_token=iam_token),
        log=logger.getChild('nbs-grpc-client')
    )
    mon_client = MonitoringClient(
        endpoint='{}:{}'.format(args.monhost, args.mon_port),
        log=logger.getChild('nbs-mon-client')
    )

    def merge_from_args_and_file(args, file):
        result = set()
        if args is not None:
            for x in args:
                result.add(x)
        if file is not None:
            with open(file, 'r') as f:
                for x in f.read().splitlines():
                    result.add(x)
        return result

    volumes = merge_from_args_and_file(args.volume, args.volumes_file)
    tablets = merge_from_args_and_file(args.tablet, args.tablets_file)
    processed_tablets = ConcurrentPersistentCompletionSet(args.completion_file)

    compacter = Compacter(
        grpc_client=grpc_client,
        mon_client=mon_client,
        processed_tablets=processed_tablets,
        compact_target=args.compact_target,
        compact_action=args.compact_action,
        aux_params=args.aux_params,
        kill_tablets=args.kill_tablets,
        logger=logger.getChild('compacter')
    )

    completed = 0
    max_completed = len(volumes) + len(tablets)

    with progressbar.ProgressBar(max_value=max_completed) as bar:
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.batch) as executor:
            future2volume = dict()
            for volume in volumes:
                future = executor.submit(Compacter.compact_volume, compacter, volume)
                future2volume[future] = volume

            for future in concurrent.futures.as_completed(future2volume):
                volume = future2volume[future]
                try:
                    if future.result():
                        logger.info('Volume has been compacted [id={}]'.format(volume))
                    else:
                        logger.warning('Skipped volume [id={}], since it\'s been already compacted'.format(volume))
                except Exception as e:
                    logger.error('Couldn\'t compact volume [id={}]: {}'.format(volume, e))
                    raise
                finally:
                    completed += 1
                    bar.update(completed)

            future2tablet = dict()
            for tablet in tablets:
                future = executor.submit(Compacter.compact_tablet, compacter, tablet)
                future2tablet[future] = tablet

            for future in concurrent.futures.as_completed(future2tablet):
                tablet = future2tablet[future]
                try:
                    if future.result():
                        logger.info('Tablet has been compacted [id={}]'.format(tablet))
                    else:
                        logger.warning('Skipped tablet [id={}], since it\'s been already compacted'.format(tablet))
                except Exception as e:
                    logger.error('Couldn\'t compact tablet [id={}]: {}'.format(tablet, e))
                finally:
                    completed += 1
                    bar.update(completed)


if __name__ == '__main__':
    main()
