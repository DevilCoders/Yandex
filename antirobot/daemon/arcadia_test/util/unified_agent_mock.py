
import re
import json
import time
import yatest
import tempfile
import subprocess
import dateutil.parser
from datetime import datetime, timedelta, timezone
from pathlib import Path

from antirobot.daemon.arcadia_test.util.mock import NetworkSubprocess
from antirobot.daemon.arcadia_test.util import asserts
from antirobot.scripts.antirobot_eventlog.event import Event
from antirobot.scripts.antirobot_cacher_daemonlog.cacher_log import CacherDaemonLog
from antirobot.scripts.antirobot_processor_daemonlog.processor_log import ProcessorDaemonLog
from google.protobuf import symbol_database


EventClasses = dict(
    (evName, evCls)
    for evName, evCls in symbol_database.Default().GetMessages(['antirobot/idl/antirobot.ev']).items()
    )

DaemonLogClasses = dict(
    (logName, logCls)
    for logName, logCls in symbol_database.Default().GetMessages(['antirobot/idl/daemon_log.proto']).items()
    )


class UnifiedAgentLog:
    DELIMITER = "6562a928-9f24-4275-819f-9dd54ccc3c93"

    def __init__(self, unified_agent, storage_name):
        self.unified_agent = unified_agent
        self.storage_name = storage_name
        self.log_time = datetime.now(timezone.utc)
        self.last_log_time = self.log_time
        self.log_directory = tempfile.TemporaryDirectory()

    def update_log_time(self):
        self.log_time = self.last_log_time

    def pop_logs(self):
        result = self.get_logs()
        self.update_log_time()
        return result

    def get_logs(self):
        process = subprocess.run(
            [self.unified_agent.path, '--config', str(Path(self.unified_agent.config.name).absolute()),
             'select', '-s', self.storage_name,
             '-o', 'ts-payload', '-d', self.DELIMITER, '-S', str(self.log_time)], capture_output=True
        )
        result = []
        for ts_event in process.stdout.split(self.DELIMITER.encode()):
            if len(ts_event) != 0:
                ts, event = ts_event.split(b' ', 1)
                self.last_log_time = datetime.fromisoformat(str(dateutil.parser.parse(ts.decode('utf-8')))) + timedelta(microseconds=1)
                result.append(json.loads(event.decode('utf-8')))
        return result


class UnifiedAgent(NetworkSubprocess):
    def __init__(self, port, **kwargs):
        self.path = yatest.common.build_path(
            "logbroker/unified_agent/bin/unified_agent",
        )

        self.evlog_time = datetime.now(timezone.utc)
        self.last_evlog_time = self.evlog_time

        self.daemonlog_time = datetime.now(timezone.utc)
        self.last_daemonlog_time = self.daemonlog_time

        self.cacher_daemonlog_time = datetime.now(timezone.utc)
        self.last_cacher_daemonlog_time = self.cacher_daemonlog_time

        self.processor_daemonlog_time = datetime.now(timezone.utc)
        self.last_processor_daemonlog_time = self.processor_daemonlog_time

        self.billing_log = UnifiedAgentLog(self, "billing_log_storage")
        self.audit_log = UnifiedAgentLog(self, "captcha_cloud_api_audit_log_storage")
        self.resource_metrics_log = UnifiedAgentLog(self, "resource_metrics_log_storage")
        self.resource_metrics_raw_log = UnifiedAgentLog(self, "resource_metrics_raw_log_storage")
        self.captcha_cloud_api_search_topic_log = UnifiedAgentLog(self, "captcha_cloud_api_search_topic_log_storage")

        self.config = tempfile.NamedTemporaryFile(mode='w')
        self.event_log_directory = tempfile.TemporaryDirectory()
        self.daemon_log_directory = tempfile.TemporaryDirectory()
        self.cacher_daemon_log_directory = tempfile.TemporaryDirectory()
        self.processor_daemon_log_directory = tempfile.TemporaryDirectory()
        self.log_output = tempfile.NamedTemporaryFile(mode='w')
        self.resource_metrics_log_output = tempfile.NamedTemporaryFile(mode='w')

        self.config.write(
f"""storages:
  - name: event_log_storage
    plugin: fs
    config:
      directory: {Path(self.event_log_directory.name).absolute()}
      max_partition_size: 50mb
      data_retention:
        by_size: 25mb
  - name: daemon_log_storage
    plugin: fs
    config:
      directory: {Path(self.daemon_log_directory.name).absolute()}
      max_partition_size: 50mb
      data_retention:
        by_size: 25mb
  - name: billing_log_storage
    plugin: fs
    config:
      directory: {Path(self.billing_log.log_directory.name).absolute()}
      max_partition_size: 50mb
      data_retention:
        by_size: 25mb
  - name: captcha_cloud_api_audit_log_storage
    plugin: fs
    config:
      directory: {Path(self.audit_log.log_directory.name).absolute()}
      max_partition_size: 50mb
      data_retention:
        by_size: 25mb
  - name: resource_metrics_raw_log_storage
    plugin: fs
    config:
      directory: {Path(self.resource_metrics_raw_log.log_directory.name).absolute()}
      max_partition_size: 50mb
      data_retention:
        by_size: 25mb
  - name: resource_metrics_log_storage
    plugin: fs
    config:
      directory: {Path(self.resource_metrics_log.log_directory.name).absolute()}
      max_partition_size: 50mb
      data_retention:
        by_size: 25mb
  - name: captcha_cloud_api_search_topic_log_storage
    plugin: fs
    config:
      directory: {Path(self.captcha_cloud_api_search_topic_log.log_directory.name).absolute()}
      max_partition_size: 50mb
      data_retention:
        by_size: 25mb
  - name: cacher_daemon_log_storage
    plugin: fs
    config:
      directory: {Path(self.cacher_daemon_log_directory.name).absolute()}
      max_partition_size: 50mb
      data_retention:
        by_size: 25mb
  - name: processor_daemon_log_storage
    plugin: fs
    config:
      directory: {Path(self.processor_daemon_log_directory.name).absolute()}
      max_partition_size: 50mb
      data_retention:
        by_size: 25mb

channels:
  - name: event_log
    channel:
      pipe:
        - storage_ref:
            name: event_log_storage
      output:
        plugin: debug
        config:
          file_name: {Path(self.log_output.name).absolute()}
          delimiter: '\n'
  - name: daemon_log
    channel:
      pipe:
        - storage_ref:
            name: daemon_log_storage
      output:
        plugin: debug
        config:
          file_name: {Path(self.log_output.name).absolute()}
          delimiter: '\n'
  - name: billing_log
    channel:
      pipe:
        - storage_ref:
            name: billing_log_storage
      output:
        plugin: debug
        config:
          file_name: {Path(self.log_output.name).absolute()}
          delimiter: '\n'
  - name: captcha_cloud_api_audit_log
    channel:
      pipe:
        - storage_ref:
            name: captcha_cloud_api_audit_log_storage
      output:
        plugin: debug
        config:
          file_name: {Path(self.log_output.name).absolute()}
          delimiter: '\n'
  - name: resource_metrics_log
    channel:
      pipe:
        - storage_ref:
            name: resource_metrics_raw_log_storage
        - filter:
            plugin: parse_metrics
            config:
              format:
                solomon_json: {'{}'}
        - filter:
            plugin: accumulate_metrics
            config:
              period: 1s
              functions:
                - match: {'"{name=smartcaptcha*}"'}
                  function: sum
              function: last
        - storage_ref:
            name: resource_metrics_log_storage
      output:
        plugin: debug
        config:
          file_name: {Path(self.resource_metrics_log_output.name).absolute()}
          delimiter: '\n'
  - name: captcha_cloud_api_search_topic_log
    channel:
      pipe:
        - storage_ref:
            name: captcha_cloud_api_search_topic_log_storage
      output:
        plugin: debug
        config:
          file_name: {Path(self.log_output.name).absolute()}
          delimiter: '\n'
  - name: cacher_daemon_log
    channel:
      pipe:
        - storage_ref:
            name: cacher_daemon_log_storage
      output:
        plugin: debug
        config:
          file_name: {Path(self.log_output.name).absolute()}
          delimiter: '\n'
  - name: processor_daemon_log
    channel:
      pipe:
        - storage_ref:
            name: processor_daemon_log_storage
      output:
        plugin: debug
        config:
          file_name: {Path(self.log_output.name).absolute()}
          delimiter: '\n'

routes:
  - input:
      plugin: grpc
      config:
        uri: localhost:{port}
    channel:
      case:
        - when:
            session:
              log_type: eventlog
          channel:
            channel_ref:
              name: event_log
        - when:
            session:
              log_type: daemonlog
          channel:
            channel_ref:
              name: daemon_log
        - when:
            session:
              log_type: billinglog
          channel:
            channel_ref:
              name: billing_log
        - when:
            session:
              log_type: captcha_cloud_api_audit_log
          channel:
            channel_ref:
              name: captcha_cloud_api_audit_log
        - when:
            session:
              log_type: resource_metrics_log
          channel:
            channel_ref:
              name: resource_metrics_log
        - when:
            session:
              log_type: captcha_cloud_api_search_topic_log
          channel:
            channel_ref:
              name: captcha_cloud_api_search_topic_log
        - when:
            session:
              log_type: cacher_daemonlog
          channel:
            channel_ref:
              name: cacher_daemon_log
        - when:
            session:
              log_type: processor_daemonlog
          channel:
            channel_ref:
              name: processor_daemon_log
        - channel:
            output:
              plugin: dev_null
""")
        self.config.flush()
        super().__init__(self.path, port, ['--config', Path(self.config.name).absolute()], **kwargs)

    def __del__(self):
        self.config.close()
        self.event_log_directory.cleanup()
        self.daemon_log_directory.cleanup()
        self.billing_log.log_directory.cleanup()
        self.audit_log.log_directory.cleanup()
        self.resource_metrics_log.log_directory.cleanup()
        self.resource_metrics_raw_log.log_directory.cleanup()
        self.captcha_cloud_api_search_topic_log.log_directory.cleanup()
        self.cacher_daemon_log_directory.cleanup()
        self.processor_daemon_log_directory.cleanup()
        self.log_output.close()
        self.resource_metrics_log_output.close()

    def update_evlog_time(self):
        self.evlog_time = self.last_evlog_time

    def apply_predicate(self, ev, predicate):
        if predicate is None:
            return True
        if isinstance(predicate, str):
            return ev.EventType == predicate
        if isinstance(predicate, (list, set, tuple)):
            return ev.EventType in predicate
        return predicate(ev)

    def pop_event_logs(self, predicate=None):
        result = self.get_event_logs(predicate)
        self.update_evlog_time()
        return result

    def get_event_logs(self, predicate=None):
        process = subprocess.run(
            [self.path, '--config', str(Path(self.config.name).absolute()), 'select', '-s', 'event_log_storage',
             '-o', 'ts-payload', '-d', 'DELIMITER', '-S', str(self.evlog_time)], capture_output=True
        )
        result = []
        for ts_event in process.stdout.split(b'DELIMITER'):
            if len(ts_event) != 0:
                ts, event = ts_event.split(b' ', 1)
                self.last_evlog_time = datetime.fromisoformat(str(dateutil.parser.parse(ts.decode('utf-8')))) + timedelta(microseconds=1)
                pbMsg = EventClasses["NAntirobotEvClass.TProtoseqRecord"]()
                pbMsg.ParseFromString(event)
                e = Event(pbMsg.event)
                if self.apply_predicate(e, predicate):
                    result.append(e)
        return result

    def get_last_event_in_verochka_logs(self):
        while True:
            result = self.pop_event_logs(["TVerochkaRecord"])

            if len(result) != 0:
                return result[-1]

            time.sleep(0.1)

    def update_daemonlog_time(self):
        self.daemonlog_time = self.last_daemonlog_time

    def pop_daemon_logs(self):
        result = self.get_daemon_logs()
        self.update_daemonlog_time()
        return result

    def get_daemon_logs(self):
        process = subprocess.run(
            [self.path, '--config', str(Path(self.config.name).absolute()), 'select', '-s', 'daemon_log_storage',
             '-o', 'ts-payload', '-d', 'DELIMITER', '-S', str(self.daemonlog_time)], capture_output=True
        )
        result = []
        for ts_event in process.stdout.split(b'DELIMITER'):
            if len(ts_event) != 0:
                ts, event = ts_event.split(b' ', 1)
                self.last_daemonlog_time = datetime.fromisoformat(str(dateutil.parser.parse(ts.decode('utf-8')))) + timedelta(microseconds=1)
                result.append(json.loads(event.decode('utf-8')))
        return result

    def get_last_event_in_daemon_logs_with_unikey(self, unique_key=None, timestamp=None):
        last_accepted = None

        while True:
            logs = self.pop_daemon_logs()

            for log in logs:
                if unique_key is None or \
                log['unique_key'] == unique_key and timestamp is None or \
                log['unique_key'] == unique_key and log['event_unixtime'] == timestamp:
                    last_accepted = log

            if last_accepted is not None:
                return last_accepted

            time.sleep(0.1)

    def wait_log_line_with_query(self, regexp, get_full_log=False):
        while True:
            logs = self.pop_daemon_logs()

            for log in reversed(logs):
                if re.match(regexp, log["req_url"]):
                    if get_full_log:
                        return logs
                    return log

            time.sleep(0.1)

    def get_last_event_in_daemon_logs(self, user_ip=None):
        last_accepted = None

        while True:
            logs = self.pop_daemon_logs()

            for log in logs:
                if user_ip is None or log.get("user_ip") == user_ip:
                    last_accepted = log

            if last_accepted is not None:
                return last_accepted

            time.sleep(0.1)

    def wait_event_logs(self, predicate, max_count=1):
        result = []
        def wait():
            events = self.pop_event_logs(predicate)
            if len(events) > 0:
                assert len(events) <= max_count, f"{len(events)} <= {max_count}"
                for e in events:
                    result.append(e)
                return True
            return False

        asserts.AssertEventuallyTrue(wait)
        return result

    def update_cacher_log_time(self):
        self.cacher_daemonlog_time = self.last_cacher_daemonlog_time

    def pop_cacher_logs(self):
        result = self.get_cacher_logs()
        self.update_cacher_log_time()
        return result

    def get_cacher_logs(self):
        process = subprocess.run(
            [self.path, '--config', str(Path(self.config.name).absolute()), 'select', '-s', 'cacher_daemon_log_storage',
             '-o', 'ts-payload', '-d', 'DELIMITER', '-S', str(self.cacher_daemonlog_time)], capture_output=True
        )
        result = []
        for ts_event in process.stdout.split(b'DELIMITER'):
            if len(ts_event) != 0:
                ts, event = ts_event.split(b' ', 1)
                self.last_cacher_daemonlog_time = datetime.fromisoformat(str(dateutil.parser.parse(ts.decode('utf-8')))) + timedelta(microseconds=1)
                pbMsg = DaemonLogClasses["NDaemonLog.TCacherRecord"]()
                pbMsg.ParseFromString(event)
                log = CacherDaemonLog(pbMsg)
                result.append(log)
        return result

    def get_last_cacher_log(self, user_ip=None):
        last_accepted = None

        while True:
            logs = self.pop_cacher_logs()

            for log in logs:
                if user_ip is None or log.user_ip == user_ip:
                    last_accepted = log

            if last_accepted is not None:
                return last_accepted

            time.sleep(0.1)

    def get_last_cacher_logs_with_unikey(self, unique_key=None, timestamp=None):
        last_accepted = None

        while True:
            logs = self.pop_cacher_logs()

            for log in logs:
                if unique_key is None or \
                log.unique_key == unique_key and timestamp is None or \
                log.unique_key == unique_key and log.timestamp == timestamp:
                    last_accepted = log

            if last_accepted is not None:
                return last_accepted

            time.sleep(0.1)

    def wait_cacher_log_line_with_query(self, regexp, get_full_log=False):
        while True:
            logs = self.pop_cacher_logs()

            for log in logs:
                if re.match(regexp, log.req_url):
                    if get_full_log:
                        return logs
                    return log

            time.sleep(0.1)

    def update_processor_log_time(self):
        self.processor_daemonlog_time = self.last_processor_daemonlog_time

    def pop_processor_logs(self):
        result = self.get_processor_logs()
        self.update_processor_log_time()
        return result

    def get_processor_logs(self):
        process = subprocess.run(
            [self.path, '--config', str(Path(self.config.name).absolute()), 'select', '-s', 'processor_daemon_log_storage',
             '-o', 'ts-payload', '-d', 'DELIMITER', '-S', str(self.processor_daemonlog_time)], capture_output=True
        )
        result = []
        for ts_event in process.stdout.split(b'DELIMITER'):
            if len(ts_event) != 0:
                ts, event = ts_event.split(b' ', 1)
                self.last_processor_daemonlog_time = datetime.fromisoformat(str(dateutil.parser.parse(ts.decode('utf-8')))) + timedelta(microseconds=1)
                pbMsg = DaemonLogClasses["NDaemonLog.TProcessorRecord"]()
                pbMsg.ParseFromString(event)
                log = ProcessorDaemonLog(pbMsg)
                result.append(log)
        return result

    def get_last_processor_log_with_unikey(self, unique_key=None, timestamp=None):
        last_accepted = None

        while True:
            logs = self.pop_processor_logs()

            for log in logs:
                if unique_key is None or \
                log.unique_key == unique_key and timestamp is None or \
                log.unique_key == unique_key and log.timestamp == timestamp:
                    last_accepted = log

            if last_accepted is not None:
                return last_accepted

            time.sleep(0.1)
