import os
from collections import defaultdict, namedtuple
from Queue import Queue, Empty as QueueEmpty
from threading import Thread

from serialize_log import unpack_time_ip


IpCounter = namedtuple('IpCounter', 'name ip_map time_window threshold')


def max_ips_in_time_window(times, ips, window_size):
    d = defaultdict(lambda: 0)
    qt = []
    qip = []
    n = 0
    for t, ip in zip(times, ips):
        d[ip] += 1
        qip.append(ip)
        qt.append(t)
        while t - qt[0] > window_size:
            qt.pop(0)
            ip = qip.pop(0)
            if d[ip] == 1:
                del d[ip]
            else:
                d[ip] -= 1
        n = max(n, len(d))
    return n


class MaxIpsCounter(object):
    def __init__(self, ip_counters):
        self.ip_counters = ip_counters
        self.ip_maps = [ip_counter.ip_map for ip_counter in ip_counters]
        self.thresholds = [ip_counter.threshold for ip_counter in ip_counters]

    def __call__(self, key, recs):
        times, Ips = zip(*[(rec['timestamp'], rec['ip']) for rec in recs])
        mapped_ips = dict((ip_map, map(ip_map, ips)) for ip_map in self.ip_maps)

        values = [max_ips_in_time_window(times, mapped_ips[ip_cnt.ip_map], ip_cnt.time_window) for ip_cnt in self.ip_counters]
        if sum(v >= t for (v, t) in zip(values, self.thresholds)):
            yield {'key': rec['key'], 'value': values}


def count_max_ips1(tweakTask, src_table, dst_table, ip_counters):
    tweakTask.RunSort(src_table, ['key', 'timestamp'])
    tweakTask.RunReduce(MaxIpsCounter(ip_counters), src_table, dst_table, reduce_by='key')


def count_max_ips_worker(tweakTask, src_prefix, dst_prefix, ip_counters, queue):
    while True:
        try:
            c = queue.get_nowait()
        except QueueEmpty:
            return
        count_max_ips1(tweakTask, os.path.join(src_prefix, c), os.path.join(dst_prefix, c), ip_counters)


def count_max_ips(tweakTask, src_prefix, dst_prefix, cookies, ip_counters, thread_num):
    q = Queue()
    for c in cookies:
        q.put(c)

    threads = [Thread(target=count_max_ips_worker, args=(tweakTask, src_prefix, dst_prefix, ip_counters, q)) for _ in xrange(thread_num)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
