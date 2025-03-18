import porto
import time
import logging


class PortoMetrics:

    def __init__(self):
        try:
            self.api = porto.Connection()
            self.container_id = self.api.GetProperty('self', 'parent')
            parent_id = self.container_id
            cores = 0
            while cores == 0 and parent_id is not None:
                cores = float(self.api.GetProperty(parent_id, 'cpu_limit').replace('c', ''))
                parent_id = self.api.GetProperty(parent_id, 'parent')
            self.cores = cores
            self.prev_ts = time.time()
            self.prev_cpu_usage = self.__get_cpu_usage()
            self.failed = False
            logging.info('container: {} parent: {} cores: {}'.format(self.container_id, parent_id, self.cores))
        except Exception:
            logging.exception("failed to init porto")
            self.failed = True

    def __get_cpu_usage(self):
        return int(self.api.GetProperty(self.container_id, 'cpu_usage')) / 1e9

    def cpu_percent(self):
        if self.failed:
            return -1
        ts = time.time()
        cpu_usage = self.__get_cpu_usage()
        cpu_diff = cpu_usage - self.prev_cpu_usage
        ts_diff = ts - self.prev_ts
        self.prev_ts = ts
        self.prev_cpu_usage = cpu_usage
        return 100.0 * cpu_diff / ts_diff / self.cores


PORTO_METRICS = PortoMetrics()
