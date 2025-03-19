#!/usr/bin/python3

import argparse
import contextlib
import random
import sys
import threading
import time

from yc_snapshot_client import *
from yc_nbs_client import *


SNAPSHOT_ENDPOINT = "localhost:7627"
TIMEOUT = 2
NULL_DISK = "null"


def monitor(client, task_id, size, index, loop=True):
    offset = 0
    last_time = time.monotonic()
    while True:
        t = time.monotonic()
        try:
            status = client.get_task_status(task_id)
        except SnapshotClientError as e:
            if 'not found' in e.message:
                offset = 0
                last_time = time.monotonic()
                time.sleep(TIMEOUT)
                continue
            raise

        speed = (status.offset - offset) / (t - last_time) / 1024 / 1024
        offset = status.offset
        last_time = t

        print("thread {:2d}: current speed: {:7.2f} MB/s, progress: {:5.2f} %".format(
                  index, speed, 100 * offset / size))
        if status.finished:
            client.delete_task(task_id)
            print("thread {:2d}: finished".format(index))
            offset = 0
            if not loop:
                return

        time.sleep(TIMEOUT)


def move(client, task_id, snapshot_id, disk_id, cluster_id):
    try:
        client.delete_task(task_id)
    except SnapshotClientError as e:
        pass

    nbs_dst = null_dst = None
    src = SnapshotMoveSrc(snapshot_id=snapshot_id)
    if disk_id == NULL_DISK:
        null_dst = NullMoveDst(size=(100<<30), block_size=(1<<12))
    else:
        nds_dst = NbsMoveDst(disk_id=disk_id, cluster_id=cluster_id)
    params = MoveParams(skip_zeroes=True, block_size=(1<<22), task_id=task_id)
    client.move(MoveRequest(snapshot_move_src=src, nbs_move_dst=nbs_dst, 
                            null_move_dst=null_dst, move_params=params))


@contextlib.contextmanager
def generate_disk_id(disk_id=None, nbs_client=None):
    if disk_id:
        yield disk_id
    elif nbs_client:
        disk_id = "lantame-" + str(random.randint(0, 100000))
        nbs_client.create_volume(disk_id, 1 << 12, 100 << 18, None, None)
        nbs_client.mount_volume(disk_id, None, None, None)
        try:
            yield disk_id
        finally:
            nbs_client.destroy_volume(disk_id, None)
    else:
        raise Exception("no disk_id and no nbs client")


def generate_task_id(cluster_id, task_id=None):
    return "test-" + cluster_id + "-" + str(random.randint(0, 100000)) if not task_id else task_id


def test(client, snapshot_id, cluster_id, task_id=None, 
         disk_id=None, nbs_client=None, count=-1, thread_count=1):
    while count:
        if task_id and thread_count > 1:
            raise Exception("for multithreading, task_id must be empty")

        def run(index):
            current_task_id = generate_task_id(cluster_id, task_id)
            with generate_disk_id(disk_id, nbs_client) as current_disk_id:
                size = client.info(snapshot_id).size
                move(client, current_task_id, snapshot_id, current_disk_id, cluster_id)
                monitor(client, current_task_id, size, index, loop=False)

        threads = [threading.Thread(target=run, args=(i,)) for i in range(thread_count)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
            
        count -= 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Test restore speed')
    parser.add_argument("--task_id")
    parser.add_argument("--snapshot_id")
    parser.add_argument("--disk_id")
    parser.add_argument("--cluster_id", default="man")
    parser.add_argument("--nbs_endpoint")
    parser.add_argument("--count", type=int, default=-1)
    parser.add_argument("--thread_count", type=int, default=1)
    args = parser.parse_args()

    with SnapshotClient(SNAPSHOT_ENDPOINT) as client:
        nbs_client = NbsClient(args.nbs_endpoint) if args.nbs_endpoint else None
        test(client=client, task_id=args.task_id, snapshot_id=args.snapshot_id,
             cluster_id=args.cluster_id, disk_id=args.disk_id,
             nbs_client=nbs_client, count=args.count, thread_count=args.thread_count)

