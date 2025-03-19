"""
Salt master metrics collector and exporter
"""

import logging
import time
import threading

try:
    import prometheus_client
except ImportError:
    prometheus_client = None

try:
    import six
except ImportError:
    six = None

import salt.key
import salt.utils.minions
import salt.utils.event
import salt.utils.process
import salt.version

log = logging.getLogger(__name__)

__virtualname__ = "prometheus_metrics"


def __virtual__():
    if prometheus_client is None:
        return False, "prometheus_client not installed"
    if six is None:
        return False, "six not installed"
    return __virtualname__


class MinionsMasterMetrics:
    """
    Salt master metrics about minions
    """

    def __init__(self, opts):
        if opts["transport"] not in ("zeromq", "tcp"):
            raise RuntimeError("transport {} not supported".format(opts["transport"]))
        self._keys = salt.key.Key(opts)
        self._ckminions = salt.utils.minions.CkMinions(opts)
        # pylint: disable=no-value-for-parameter
        self.master_keys = prometheus_client.Gauge(
            "salt_master_keys", "Number of salt-master minions key", ["state"]
        )
        self.connected_minions = prometheus_client.Gauge(
            "salt_master_connected_minions", "Number of salt-master minions"
        )

    def update(self):
        keys = self._keys.list_keys()
        connected = self._ckminions.connected_ids()
        for state, seq in six.iteritems(
            {
                "accepted": keys.get("minions"),
                "pending": keys.get("minions_pre"),
                "denied": keys.get("minions_denied"),
                "rejected": keys.get("minions_rejected"),
            }
        ):
            # pylint: disable=no-member
            self.master_keys.labels(state).set(len(seq) if seq else 0)
        self.connected_minions.set(len(connected))


class EventMasterMetrics(threading.Thread):
    tribool_conv = {
        True: "true",
        False: "false",
        None: "unknown",
    }

    def __init__(self, opts):
        super(  # pylint: disable=super-with-arguments
            EventMasterMetrics, self
        ).__init__()
        # Use function level import,
        # cause we shouldn't fail if six not installed
        from .event import EventConnoisseur  # pylint: disable=import-outside-toplevel

        self._event_connoisseur = EventConnoisseur()
        self._event_bus = salt.utils.event.get_event(
            opts.get("__role", "master"),
            transport=opts.get("transport", "zeromq"),
            opts=opts,
            listen=True,
        )
        self.event = prometheus_client.Counter(
            "salt_master_event", "Count of salt-master events", ["tag"]
        )
        self.func = prometheus_client.Counter(
            "salt_master_func",
            "Count of function calls",
            ["func", "success", "retcode"],
        )
        self.state = prometheus_client.Counter(
            "salt_master_state_sls",
            "Count of state.sls calls",
            ["state", "success", "retcode"],
        )

    def _on_event(self, finger):
        if finger is not None:
            self.event.labels(finger.tag).inc()  # pylint: disable=no-member
            if finger.func:
                # Some functions don't have retcode
                # {u'tag': 'salt/run/20210713173157274548/ret',
                #  u'data': {u'fun_args': [u'worker_threads'], u'jid': u'20210713173157274548',
                #            u'return': 16,
                #            u'success': True, u'_stamp': u'2021-07-13T17:31:58.660214',
                #            u'user': u'mdb-deploy-salt-api',
                #            u'fun': u'runner.config.get'}
                #
                # But we should specify 'some' for metric label
                retcode = "unknown" if finger.retcode is None else finger.retcode
                success = self.tribool_conv.get(finger.success)
                self.func.labels(  # pylint: disable=no-member
                    finger.func, success, retcode
                ).inc()
                if finger.state:
                    self.state.labels(  # pylint: disable=no-member
                        finger.state, success, retcode
                    ).inc()

    def run(self):
        for event in self._event_bus.iter_events(full=True):
            finger = self._event_connoisseur.get_finger(event)
            if finger is not None:
                self._on_event(finger)


class SaltInfo:
    def __init__(self):
        self.metric = prometheus_client.Gauge(
            "salt_info", "Salt information", ["name", "major", "minor", "version"]
        )  # pylint: disable=no-member

        ver = salt.version.__saltstack_version__
        self.metric.labels(
            ver.name, ver.major, ver.minor, salt.version.__version__
        ).set(1)


class Collector:
    """
    Salt master metrics
    """

    def __init__(self, opts, update_interval, port):
        self.update_interval = update_interval
        self.port = port
        self.minions = MinionsMasterMetrics(opts)
        self.events = EventMasterMetrics(opts)
        self.info = SaltInfo()

    def start(self):
        # Start prometheus server
        prometheus_client.start_http_server(self.port)
        self.events.start()
        while True:
            step_start_at = time.time()
            self.minions.update()
            time.sleep(time.time() + self.update_interval - step_start_at)


def start(update_interval=15, port=6060):
    salt.utils.process.appendproctitle("PrometheusMetricsCollector")
    collector = Collector(
        __opts__,  # pylint: disable=undefined-variable
        update_interval,
        port,
    )
    collector.start()
