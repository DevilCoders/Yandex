import bisect
import logging
import threading
from datetime import datetime, timedelta
from collections import deque, defaultdict

import flask

logger = logging.getLogger(__name__)

DEFAULT_AGENT_PORT = 5005
QUEUE_MAXSIZE = 10000


def avg(arr):
    return sum(float(x) / len(arr) for x in arr)


class Kind:
    IGAUGE = 'IGAUGE'
    DGAUGE = 'DGAUGE'
    COUNTER = 'COUNTER'
    RATE = 'RATE'
    HIST = 'HIST'
    HIST_RATE = 'HIST_RATE'


class HistogramType:
    EXPONENTIAL = 0
    LINEAR = 1
    EXPLICIT = 2


class Histogram:
    def __init__(self, labels, **kwargs):
        self.labels = labels
        self.params = kwargs
        if self.params['hist_type'] == HistogramType.LINEAR:
            self.bounds = [int(self.params['start_value'] + i * self.params['bucket_width']) for i in range(self.params['bucket_count']-1)]
        elif self.params['hist_type'] == HistogramType.EXPONENTIAL:
            self.bounds = [int(self.params['scale'] * pow(self.params['base'], i)) for i in range(self.params['bucket_count']-1)]
        elif self.params['hist_type'] == HistogramType.EXPLICIT:
            self.bounds = self.params['bounds']
            self.params['bucket_count'] = len(self.params['bounds']) + 1
        else:
            raise Exception("Add some code for type: {}".format(self.params['hist_type']))
        self.buckets = [0] * self.params['bucket_count']

    def collect(self, value):
        bucket_id = bisect.bisect_left(self.bounds, int(value))
        bucket_id = min(self.params['bucket_count'] - 1, bucket_id)
        self.buckets[bucket_id] += 1

    def serialize(self):
        return {
            'labels': self.labels,
            'type': Kind.HIST,
            'hist': {
                'bounds': self.bounds,
                'buckets': self.buckets[:-1],
                'inf': self.buckets[-1]
            }
        }


HIST_SETTINGS = {
    # 0, 50KB, 250KB, 500KB, 750KB, 1MB, 2MB, 3MB, 5MB, 7MB, 10MB, inf
    'progress.speed_in_bytes': {'hist_type': HistogramType.EXPLICIT, 'bounds': [0, 51200, 256000, 512000, 768000, 1048576,
                                                                                2097152, 3145728, 5242880, 7340032, 10485760]},
    'completed.duration': {'hist_type': HistogramType.EXPLICIT, 'bounds': [1, 2, 3, 5, 8, 15, 30, 60, 90,
                                                                           150, 300, 450, 600, 750, 900, 1100, 2000]}
}


class Metric:
    def __init__(self, labels, value, kind, ts):
        self.labels = labels
        self.kind = kind
        self.ts = ts
        self.values = [value]

    def get_label_value(self, name):
        return self.labels[name]

    def get_value(self):
        return self.values[0]

    def extend_metric(self, value):
        self.values += [value]

    def serialize(self, func):
        """
        :func - method to calculate common value for a sensor
        """
        return {
            'labels': self.labels,
            'value': func(self.values),
            'type': self.kind,
            'ts': self.ts.strftime('%Y-%m-%dT%H:%M:%SZ')
        }


class DummyAgent:
    def push_metric(self, *args, **kwargs):
        pass

    def start(self):
        pass


class SolomonAgent:
    hist_metrics = defaultdict(dict)

    metrics = deque(maxlen=QUEUE_MAXSIZE)
    grid = 1  # in seconds
    _aggr_methods = {}
    _ignore_sensors = set()

    def __init__(self, port=DEFAULT_AGENT_PORT, yabs_mode=False):
        self.yabs_mode = yabs_mode

        self.app = flask.Flask(__name__, static_folder=None, template_folder=None)
        self.app.add_url_rule('/sensors', 'sensors', self.pull_view)
        self.flask_thread = threading.Thread(target=self.app.run, kwargs={'host': '::', 'port': port})
        self.flask_thread.daemon = True

    def start(self):
        self.flask_thread.start()

    def ignore_sensors(self, sensors):
        self._ignore_sensors.update(sensors)

    def push_metric(self, sensor, labels, value, kind=Kind.DGAUGE, ts=None):
        """
        :name: sensor name
        https://wiki.yandex-team.ru/solomon/api/dataformat/json/
        """
        try:
            if sensor in self._ignore_sensors:
                return
            metric_labels = {'sensor': sensor}
            metric_labels.update(labels or {})

            if self.yabs_mode:
                if sensor in HIST_SETTINGS:
                    base_name = labels['base_qualified_name']
                    if base_name not in self.hist_metrics[sensor]:
                        self.hist_metrics[sensor][base_name] = Histogram(metric_labels, **HIST_SETTINGS[sensor])

                    self.hist_metrics[sensor][base_name].collect(value)
                    return

            ts = ts or datetime.utcnow().replace(microsecond=0)
            self.metrics.append(Metric(labels=metric_labels, value=value, kind=kind, ts=ts))
        except Exception as e:
            logger.error("Failed pushing metric to queue: %s", e, exc_info=True)

    def set_grid(self, value):
        self.grid = value

    def set_aggr_method(self, sensor, func):
        """
        Sets function to aggregate values grouped by grid
        If not set, by default will be used max function. See get_aggr_method()
        """
        self._aggr_methods[sensor] = func

    def get_aggr_method(self, sensor):
        return self._aggr_methods.get(sensor, max)

    def pull_view(self):
        result = []
        cnt = 0
        while self.metrics and cnt < QUEUE_MAXSIZE:
            cnt += 1
            metric = self.metrics.popleft()

            found = False
            for m in reversed(result):
                if metric.ts - m.ts >= timedelta(seconds=self.grid):
                    break
                if m.labels == metric.labels:
                    found = True
                    m.extend_metric(metric.get_value())
                    break

            if not found:
                result.append(metric)

        sensors = []
        for metric in result:
            aggr_func = self.get_aggr_method(metric.get_label_value('sensor'))
            sensors.append(metric.serialize(aggr_func))

        logger.info("Pulling metrics data for Solomon. Data count - before aggr: {}, after aggr: {}"
                    .format(cnt, len(sensors)))

        if self.yabs_mode:
            for sensor in self.hist_metrics.keys():
                for base_name in tuple(self.hist_metrics[sensor].keys()):
                    hist = self.hist_metrics[sensor].pop(base_name)
                    sensors.append(hist.serialize())

        return flask.jsonify({'sensors': sensors})
