import json
import logging
import multiprocessing.dummy
import threading
import time
from datetime import datetime
import enum

import furl
import requests.packages.urllib3
import six
from six.moves import queue
from six.moves import urllib


LOGGER = logging.getLogger(__name__)


def seconds_bucket_to_str(bucket):
    """
    :param timedelta bucket: Time bucket to create string representation for
    :rtype: str
    """
    return '{}s'.format(int(bucket.total_seconds()))


class SysmondReporter(object):
    default_reporter = None

    def __init__(self, sysmond_url='http://localhost:3443/update', service_name='test'):
        """
        :param str sysmond_url:
        :param str service_name:
        """
        self._sysmond_url = sysmond_url
        self._service_name = service_name

    @classmethod
    def init_default_reporter(cls, sysmond_url='http://localhost:3443/update', service_name='test'):
        cls.default_reporter = SysmondReporter(sysmond_url, service_name)

    def send_user_stat(self, sensor_name, value,
                       operation=None, labels=None, is_derived=False, ttl=None):
        params = {
            'service': self._service_name,
            'sensor': sensor_name,
            'value': str(value)
        }
        if operation:
            params['op'] = str(operation)
        if labels:
            for label_key, label_value in six.iteritems(labels):
                params['label.{}'.format(label_key)] = str(label_value)
        if ttl is not None:
            params['expire'] = str(ttl)
        if is_derived:
            params['kind'] = 'RATE'

        self._make_request(params)

    def _make_request(self, params):
        url = '{}?{}'.format(self._sysmond_url, urllib.parse.urlencode(params))
        requests.get(url)

    def set_value(self, sensor_name, value, labels=None, is_derived=False, ttl=None):
        return self.send_user_stat(sensor_name, value,
                                   labels=labels, is_derived=is_derived, ttl=ttl)


class OAuthProvider(object):
    def __init__(self, token):
        self.token = token

    def put_headers(self, headers):
        headers['Authorization'] = 'OAuth %s' % self.token


@enum.unique
class SensorNameLabel(str, enum.Enum):
    SENSOR = 'sensor'
    NAME = 'name'
    SIGNAL = 'signal'
    PATH = 'path'
    METRIC = 'metric'


class BasePushApiReporter(object):

    DATE_FMT = '%Y-%m-%dT%H:%M:%SZ'
    PUSH_URL_PATH = '/push'
    PUSH_URL_PATH_V2 = '/api/v2/push'
    DEPRECATED_PUSH_URL_PATH = '/push/json'

    def __init__(
        self,
        project,
        cluster,
        service,
        url='http://solomon-prestable.yandex.net',
        timeout=60.0,
        common_labels=None,
        auth_provider=None,
        sensor_name_label=SensorNameLabel.SENSOR,
        request_auth_provider=None,
    ):
        """
        :param str project:
        :param str cluster:
        :param str service:
        :param str push host or url:
        :param float request timeout:
        :param dict common labels:
        :param object auth_provider:
        :param str sensor_name_label:
        :param object request_auth_provider:
        """
        self.url = url
        self.project = project
        self.cluster = cluster
        self.service = service
        self.common_labels = common_labels if common_labels is not None else {}
        self.timeout = timeout
        self.auth_provider = auth_provider
        self.request_auth_provider = request_auth_provider
        self.sensor_name_label = SensorNameLabel(sensor_name_label)

        self.push_url = self._make_push_url(url)
        self._session = self._make_session()

    def _make_session(self, retries=3, backoff_factor=0.1, pool_connections=1, pool_maxsize=10):
        session = requests.Session()
        session.headers.update({'Content-Type': 'application/json'})
        if self.auth_provider is not None:
            self.auth_provider.put_headers(session.headers)

        retry = requests.packages.urllib3.Retry(
            total=retries,
            backoff_factor=backoff_factor,
            status_forcelist=[429, 500, 502, 503, 504],
        )
        adapter = requests.adapters.HTTPAdapter(
            max_retries=retry,
            pool_connections=pool_connections,
            pool_maxsize=pool_maxsize,
        )
        session.mount('http://', adapter)
        session.mount('https://', adapter)
        return session

    def _make_push_url(self, base_url):
        push_furl = furl.furl(base_url)

        if not push_furl.path:
            push_furl.set(path=self.PUSH_URL_PATH_V2 if self.auth_provider is not None or self.request_auth_provider is not None else self.PUSH_URL_PATH)

        path = str(push_furl.path.normalize()).rstrip('/')
        push_furl.set(path=path)

        if path in [self.PUSH_URL_PATH, self.PUSH_URL_PATH_V2]:
            push_furl.set(
                query_params={
                    'project': self.project,
                    'cluster': self.cluster,
                    'service': self.service,
                },
            )
        elif path == self.DEPRECATED_PUSH_URL_PATH:
            self.common_labels.update({
                'project': self.project,
                'cluster': self.cluster,
                'service': self.service,
            })

        return push_furl.url

    def set_value(self, sensor, value, labels=None, is_derived=False, ts_datetime=None):
        """
        Set value for one or multiple sensors.

        :param str|list[str] name:
        :param int|double|list[int]|list[double] value:
        :param None|dict|list[dict] labels:
        :param None|bool|list[bool] is_derived:
        :param None|datetime|list[datetime] ts_datetime:
        """
        if isinstance(sensor, six.string_types):
            sensor = [sensor]
            value = [value]
            if labels:
                labels = [labels]
            if is_derived:
                is_derived = [is_derived]
            if ts_datetime:
                ts_datetime = [ts_datetime]
        if not labels:
            labels = [{} for _ in sensor]
        if not is_derived:
            is_derived = [False for _ in sensor]
        if not ts_datetime:
            current_time = self._get_current_time()
            ts_datetime = [current_time for _ in sensor]
        if not isinstance(ts_datetime, list):
            ts_datetime = [ts_datetime for _ in sensor]
        payload = self._make_payload(sensor, value, labels, is_derived, ts_datetime)
        self._push(payload)

    def _make_payload(self, sensors, values, labels, is_derived, ts_datetime):
        """
        :param list[str] sensor names:
        :param list[int]|list[double] sensor values:
        :param list[dict] extra labels:
        :param list[bool] whether to use line's derivative:
        :param list[datetime] ts_datetime:
        """

        payload = {
            'sensors': [],
        }

        if self.common_labels:
            payload.update({
                'commonLabels': self.common_labels,
            })

        for i, sensor in enumerate(sensors):
            value = values[i]
            sensor_labels = labels[i]
            kind = 'RATE' if is_derived[i] else None
            ts_str = ts_datetime[i].strftime(self.DATE_FMT)

            sensor_payload = {
                'labels': {
                    self.sensor_name_label: sensor,
                },
                'ts': ts_str,
                'value': value,
            }
            sensor_payload['labels'].update(sensor_labels)
            if kind:
                sensor_payload['kind'] = kind

            payload['sensors'].append(sensor_payload)

        return payload

    def _get_current_time(self):
        return datetime.utcnow()

    def _push(self, payload):
        """
        Send a push request to Solomon server.

        :param dict sensor data to push:
        """
        return self._do_push(payload)

    def _do_push(self, payload):
        try:
            headers = self.request_auth_provider.get_request_headers() if self.request_auth_provider is not None else {}
            data = json.dumps(payload)
            try:
                response = self._session.post(
                    self.push_url,
                    data=data,
                    headers=headers,
                    timeout=self.timeout,
                )
            except requests.ConnectionError:
                # Retry connection resets by the server.
                response = self._session.post(
                    self.push_url,
                    data=data,
                    timeout=self.timeout,
                )
        except Exception as e:
            self.on_push_failed(exc=e)
            return

        try:
            response.raise_for_status()
        except Exception as e:
            self.on_push_failed(exc=e, response=response)
            return

    def on_push_failed(self, exc=None, response=None):
        '''
        To be overriden by end users for error handing.

        `response` parameter is either None in case of network errors
            or a requests.Response instance in case of a bad response.
        '''
        exc_info = exc is not None
        if response is None:
            logging.error(u'Push failed: network error', exc_info=exc_info)
        else:
            logging.error(
                u'Push failed: response code %s\n%s',
                response.status_code,
                response.text,
                exc_info=exc_info,
            )


class PushApiReporter(BasePushApiReporter):

    def __init__(self, *args, **kwargs):
        super(PushApiReporter, self).__init__(*args, **kwargs)
        self._thread_pool = multiprocessing.dummy.Pool()

    def _push(self, payload):
        self._thread_pool.apply_async(self._do_push, args=(payload,))


class ThrottledPushApiReporter(BasePushApiReporter):
    """A reporter that batches operations and applies them using a single request."""

    def __init__(self, push_interval, *args, **kwargs):
        super(ThrottledPushApiReporter, self).__init__(*args, **kwargs)
        self.push_interval = push_interval
        self._push_queue = queue.Queue()
        self._pusher_thread = threading.Thread(target=self._loop)
        self._pusher_thread.setDaemon(True)
        self._pusher_thread.start()

    def _push(self, payload):
        self._push_queue.put(payload)

    def _loop(self):
        while True:
            aggregated_payload = {
                'sensors': [],
            }

            if self.common_labels:
                aggregated_payload.update({
                    'commonLabels': self.common_labels,
                })

            while not self._push_queue.empty():
                payload = self._push_queue.get()
                aggregated_payload['sensors'].extend(payload['sensors'])

            if aggregated_payload['sensors']:
                self._do_push(aggregated_payload)

            time.sleep(self.push_interval)


class Sensor(object):
    def __init__(self, name, labels=None, is_derived=False, ttl=None, reporter=None):
        """
        :param str name:
        :param None|dict labels:
        :param bool is_derived:
        :param int|None ttl: Number of seconds, that this sensor will live without updates
        :param SysmondReporter|None reporter: If reporter is not set, then default reporter is taken
        """
        self._name = name
        self._is_derived = is_derived
        self._ttl = ttl
        self._labels = labels
        self._reporter = reporter if reporter else SysmondReporter.default_reporter

    def set_value(self, value):
        """
        Set new value for a sensor
        :param int|double value:
        """
        self._reporter.send_user_stat(sensor_name=self._name, value=value, labels=self._labels,
                                      is_derived=self._is_derived, ttl=self._ttl, operation='set')

    def inc_value(self, amount=1):
        """
        AtomicAdd amount to current sensor's value
        :param int|double amount:
        """
        self._reporter.send_user_stat(sensor_name=self._name, value=amount, labels=self._labels,
                                      is_derived=self._is_derived, ttl=self._ttl, operation='add')


class SensorTimeDistribution(object):
    def __init__(self, name_prefix, time_grid, bucket_to_str=seconds_bucket_to_str,
                 labels=None, is_derived=False, ttl=None, reporter=None):
        """
        :param str name_prefix:
        :param list[timedelta] time_grid:
        :param dict[str, str] | None labels:
        :param bool is_derived:
        :param int | None ttl: Number of seconds, that this sensor will live without updates
        :param SysmondReporter|None reporter: If reporter is not set, then default reporter is taken
        """
        assert time_grid
        self._time_grid = time_grid
        self._buckets = [
            (_, Sensor(name_prefix + bucket_to_str(_), labels, is_derived, ttl, reporter))
            for _ in time_grid
        ]

    def report_event(self, duration):
        """
        :param timedelta duration:
        """
        for bucket, sensor in self._buckets:
            if bucket >= duration:
                sensor.inc_value(1)
                return
        self._buckets[-1][1].inc_value(1)


def execution_time_hist(histogram):
    """
    :param SensorTimeDistribution histogram:
    """

    def decorator(f):
        def wrapper(*args, **kwargs):
            start = datetime.now()
            result = f(*args, **kwargs)
            histogram.report_event(datetime.now() - start)
            return result

        return wrapper

    return decorator


SysmondReporter.init_default_reporter()
