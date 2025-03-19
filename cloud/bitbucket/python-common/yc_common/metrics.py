import collections
import functools
import prometheus_client as prom
import threading

from prometheus_client import parser

from yc_common import config
from yc_common.exceptions import Error, LogicalError

from yc_common import logging

log = logging.get_logger(__name__)
time_buckets_ms = (10.0, 50.0, 100.0, 500.0, 750.0,
                   1000.0, 2500.0, 5000.0, 7500.0,
                   10000.0, 25000.0, 50000.0, float("inf"))


class MetricTypes:
    COUNTER = "counter"
    GAUGE = "gauge"
    HISTOGRAM = "histogram"
    ALL = [COUNTER, GAUGE, HISTOGRAM]


class MetricFormat:
    RAW = "raw"
    JSON = "json"
    ALL = [RAW, JSON]


def _compose_labels(labels, label_names, **kwargs):
    if labels is None:
        labels = {}

    label_values = []
    for label in label_names:
        if label in kwargs:
            label_values.append(kwargs[label])
        elif label in labels:
            label_values.append(labels[label])
        else:
            raise LogicalError("Label {!r} is not present.", label)
    return label_values


def monitor(metric, labels=None):
    def decorator(func):
        request_labels = _compose_labels(labels, metric.label_names, type="request")
        error_labels = _compose_labels(labels, metric.label_names, type="error")

        if metric.type != MetricTypes.COUNTER:
            raise LogicalError("Unable to use metric type {!r} in wrapper.", metric.type)

        @functools.wraps(func)
        def wrap(*args, **kwargs):
            metric.labels(*request_labels).inc()
            try:
                return func(*args, **kwargs)
            except:
                metric.labels(*error_labels).inc()
                raise
        return wrap
    return decorator


def wrap_methods_with_counter(obj, methods, metric, label_fn):
    for method_name in set(methods).intersection(dir(obj)):
        wrapper = monitor(metric, label_fn(method_name))
        wrapped_method = wrapper(getattr(obj, method_name))
        setattr(obj, method_name, wrapped_method)
    return obj


class _LocalRegistry(object):
    def __init__(self):
        self.__metrics = {}

    def add(self, name, mtype, labels, obj):
        if name in self.__metrics:
            raise Error("Metric {} already present in registry.", name)
        self.__metrics[name] = {"type": mtype, "labels": labels, "object": obj}

    def get(self, name, mtype, labels):
        metric = self.__metrics.get(name)
        if metric is None:
            return None

        if set(metric["labels"]) != set(labels):
            raise Error("Metric {} has another label set: {} {}.", name, metric["labels"], labels)

        if metric["type"] != mtype:
            raise Error("Metric {} has another type: {} {}.", name, metric["type"], mtype)

        return metric["object"]

    def exists(self, name):
        return name in self.__metrics

    def reset(self, name):
        metric = self.__metrics.get(name, None)
        if metric is not None:
            try:
                prom.REGISTRY.unregister(metric["object"])
            except Exception as e:
                raise Error("Unable to reset metric: {}.", e)

            self.__metrics.pop(name, None)


_local_registry = _LocalRegistry()
_registry_lock = threading.Lock()


def collect(encode=MetricFormat.RAW):
    """
    This code explicitly falls back from OpenMetrics model prior to 0.4.0.
    Cause these are expectations.

    See comment in commit.
    """
    if encode not in MetricFormat.ALL:
        raise Error("Unsupported encoding format: {}.", encode)

    raw_metrics = prom.generate_latest()

    if encode == MetricFormat.JSON:
        result = {}
        for metric_family in parser.text_string_to_metric_families(raw_metrics.decode("ascii")):
            for sample in metric_family.samples:
                name = sample.name
                labels = sample.labels
                value = sample.value
                # name, labels, value = sample
                result.setdefault(name, []).append((labels, value))
    else:
        result = raw_metrics
    seen__created = {}
    seen__total = {}
    CREATED_SUFFIX = '_created'
    TOTAL_SUFFIX = '_total'

    for key in result:
        if key.endswith(CREATED_SUFFIX):
            seen__created[key[:-len(CREATED_SUFFIX)]] = True
        elif key.endswith(TOTAL_SUFFIX):
            seen__total[key[:-len(TOTAL_SUFFIX)]] = True
    for key in seen__created:
        if key in seen__total:
            del result[key + CREATED_SUFFIX]
            result[key] = result[key + TOTAL_SUFFIX]
            del result[key + TOTAL_SUFFIX]

    return result


def reset(metric_name):
    try:
        with _registry_lock:
            _local_registry.reset(metric_name)
    except Error as e:
        log.error("Unable to reset %r: %s", metric_name, e)
        return False
    else:
        return True


def exponential_buckets(lower_bound, factor, count):
    if count < 1:
        raise Error("Bucket count must be at least one.")
    if lower_bound <= 0:
        raise Error("Lower bound must be non negative.")
    if factor <= 1:
        raise Error("Factor must be greater than one.")
    buckets = [float(lower_bound) * factor ** x for x in range(count)]
    return tuple(buckets + [float("inf")])


class Metric(object):
    def __init__(self, metric_type, name, label_names=None, doc="", **kwargs):
        if metric_type not in MetricTypes.ALL:
            raise Error("Unknown metric type: {}.", metric_type)
        self.type = metric_type
        self.name = name
        self.help = doc
        self.__kwargs = kwargs
        if label_names is not None:
            non_unique_labels = [label for label, count in collections.Counter(label_names).items() if count > 1]
            if non_unique_labels:
                raise Error("Non unique label names: {}.", ",".join(non_unique_labels))
        else:
            label_names = []
        self.label_names = label_names

    def __getattr__(self, attr):
        if self.label_names:
            raise AttributeError("{!r} object has no attribute {!r}".format(self, attr))
        if self._common_labels:
            return getattr(self.labels(), attr)
        return getattr(self.metric, attr)

    @property
    def _common_labels(self):
        try:
            common_labels = config.get_value("metrics.labels", default={})
        except Error:
            return {}
        return common_labels

    @property
    def _labels(self):
        diff = set(self._common_labels).intersection(self.label_names)
        if diff:
            raise Error("Label(s) are not unique: {}.", ",".join(diff))
        return self.label_names + sorted(self._common_labels)

    def _create_metric(self):
        if self.type == MetricTypes.COUNTER:
            cls = prom.Counter
        elif self.type == MetricTypes.GAUGE:
            cls = prom.Gauge
        elif self.type == MetricTypes.HISTOGRAM:
            cls = prom.Histogram
        try:
            metric = cls(self.name, self.help, self._labels, **self.__kwargs)
        except ValueError as err:
            raise Error("Metric creation failed: {}.", err)
        return metric

    @property
    def metric(self):
        with _registry_lock:
            metric = _local_registry.get(self.name, self.type, self.label_names)
            if not metric:
                metric = self._create_metric()
                _local_registry.add(self.name, self.type, self.label_names, metric)
        return metric

    def labels(self, *args):
        kw_labels = dict(zip(self.label_names, args))
        kw_labels.update(self._common_labels)
        try:
            return self.metric.labels(**kw_labels)
        except ValueError as e:
            raise Error("Label error: {}.", e)
