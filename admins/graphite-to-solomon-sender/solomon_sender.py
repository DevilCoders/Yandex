import argparse
import errno
import json
import logging
import os
import pickle
import socket
import time
from collections import namedtuple
from concurrent.futures import ThreadPoolExecutor
from enum import Enum
from logging.handlers import RotatingFileHandler
from pathlib import Path

from tornado import httpclient, gen
from tornado.httpclient import HTTPRequest
from tornado.httputil import HTTPHeaders
from tornado.ioloop import IOLoop, PeriodicCallback
from tornado.iostream import StreamClosedError
from tornado.tcpclient import TCPClient
from tornado.tcpserver import TCPServer

executor = ThreadPoolExecutor(1)

log = logging.getLogger(__name__)


class SingleWriterMultipleReaderRingBuffer:
    def write(self, item):
        raise NotImplementedError

    def get_data_loss(self):
        raise NotImplementedError

    def scan(self, reader_id, limit):
        raise NotImplementedError

    def shift_read_offset(self, reader_id, shift):
        raise NotImplementedError

    def has_data_for(self, reader_id):
        raise NotImplementedError

    def register_readers(self, reader_id):
        raise NotImplementedError

    def check_health(self):
        raise NotImplementedError

    def get_healthcheck(self):
        raise NotImplementedError

    def shallow_copy(self):
        raise NotImplementedError


class CompactingSingleWriterMultipleReaderRingBuffer(SingleWriterMultipleReaderRingBuffer):
    def __init__(self, length, decoder, healthcheck, backed_ring_buffer=None):
        self._backed_ring_buffer = backed_ring_buffer if backed_ring_buffer else SimpleSingleWriterMultipleReaderRingBuffer(
            length, healthcheck)
        self._decoder = decoder

    def write(self, item):
        self._backed_ring_buffer.write(self._decoder.encode(item))

    def get_data_loss(self):
        return self._backed_ring_buffer.get_data_loss

    def scan(self, reader_id, limit):
        return [self._decoder.decode(itm) for itm in self._backed_ring_buffer.scan(reader_id, limit)]

    def shift_read_offset(self, reader_id, shift):
        self._backed_ring_buffer.shift_read_offset(reader_id, shift)

    def has_data_for(self, reader_id):
        return self._backed_ring_buffer.has_data_for(reader_id)

    def register_readers(self, reader_ids):
        self._backed_ring_buffer.register_readers(reader_ids)

    def check_health(self):
        self._backed_ring_buffer.check_health()

    def get_healthcheck(self):
        return self._backed_ring_buffer.get_healthcheck()

    def shallow_copy(self):
        decoder = self._decoder.shallow_copy()
        buffer = self._backed_ring_buffer.shallow_copy()
        return CompactingSingleWriterMultipleReaderRingBuffer(None, decoder, None, buffer)


class SimpleSingleWriterMultipleReaderRingBuffer(SingleWriterMultipleReaderRingBuffer):
    WARN_NOT_SENDED_METRICS_PART = 0.7

    def __init__(self, length, healthcheck, buffer=None, write_offset=None, reader_id_to_last_data_loss=None,
                 reader_id_to_read_offset=None):
        self._healthcheck = healthcheck
        if buffer:
            self._buffer = buffer
            self._reader_id_to_read_offset = reader_id_to_read_offset
            self._write_offset = write_offset
            self._reader_id_to_last_data_loss = reader_id_to_last_data_loss
        else:
            self._buffer = [None] * length
            self._reader_id_to_read_offset = {}  # position of the next read
            self._write_offset = 0  # position of the next write
            self._reader_id_to_last_data_loss = {}

    def write(self, item):
        self._buffer[self._write_offset % len(self._buffer)] = item
        self._write_offset += 1

    def check_data_loss(self, reader_id):
        reader_offset = self._reader_id_to_read_offset[reader_id]
        if reader_offset + len(self._buffer) < self._write_offset:
            self._reader_id_to_last_data_loss[reader_id] = time.time()

    def get_data_loss(self):
        for reader_id in self._reader_id_to_read_offset.keys():
            self.check_data_loss(reader_id)
        return self._reader_id_to_last_data_loss

    def fix_reader_offset(self, reader_id):
        self._reader_id_to_read_offset[reader_id] = max(
            self._reader_id_to_read_offset[reader_id],
            self._write_offset - len(self._buffer)
        )

    def scan(self, reader_id, limit):
        self.check_data_loss(reader_id)
        self.fix_reader_offset(reader_id)
        read_from = self._reader_id_to_read_offset[reader_id] % len(self._buffer)
        read_to = min(
            self._write_offset,
            self._reader_id_to_read_offset[reader_id] + limit
        ) % len(self._buffer)

        if read_to <= read_from:
            return self._buffer[read_from:] + self._buffer[:read_to]

        return self._buffer[read_from:read_to]

    def shift_read_offset(self, reader_id, shift):
        self._reader_id_to_read_offset[reader_id] = min(
            self._write_offset,
            self._reader_id_to_read_offset[reader_id] + shift
        )

    def has_data_for(self, reader_id):
        return self._write_offset > self._reader_id_to_read_offset[reader_id]

    def register_readers(self, reader_ids):
        for existed_reader_id in list(self._reader_id_to_read_offset.keys()):
            if existed_reader_id not in reader_ids:
                self._reader_id_to_read_offset.pop(existed_reader_id, None)

        for reader_id in reader_ids:
            if reader_id not in self._reader_id_to_read_offset:
                self._reader_id_to_read_offset[reader_id] = self._write_offset

        for data_loss_reader_id in list(self._reader_id_to_last_data_loss.keys()):
            if data_loss_reader_id not in self._reader_id_to_read_offset:
                self._reader_id_to_last_data_loss.pop(data_loss_reader_id, None)

    def get_readers_offsets(self):
        return self._reader_id_to_read_offset.copy()

    def get_last_data_loss(self):
        return self._reader_id_to_last_data_loss.copy()

    def check_health(self):
        now = time.time()
        error_message_data_loss = ""
        for sender_id, last_loss in self.get_data_loss().items():
            if abs(now - last_loss) < 1800:
                error_message_data_loss += "Data loss. SenderId: {};".format(sender_id)
            else:
                self._reader_id_to_last_data_loss.pop(sender_id, None)
        if error_message_data_loss:
            self._healthcheck.error(error_message_data_loss)
            return
        warn_message_send_delay = ""
        for sender_id, reader_offset in self._reader_id_to_read_offset.items():
            delay = self._write_offset - reader_offset
            if delay > SimpleSingleWriterMultipleReaderRingBuffer.WARN_NOT_SENDED_METRICS_PART * len(self._buffer):
                warn_message_send_delay += "SenderId {}. Data send delay {}.".format(sender_id, delay)

        if warn_message_send_delay:
            self._healthcheck.warn(warn_message_send_delay)

    def get_healthcheck(self):
        return self._healthcheck

    def shallow_copy(self):
        return SimpleSingleWriterMultipleReaderRingBuffer(
            None, self._healthcheck, self._buffer[:], self._write_offset,
            self._reader_id_to_last_data_loss.copy(),
            self._reader_id_to_read_offset.copy()
        )


class GraphiteReceiverServer(TCPServer):
    def __init__(self, ring_buffer):
        super().__init__()
        self._ring_buffer = ring_buffer

    @gen.coroutine
    def handle_stream(self, stream, address):
        prev_read_metric = b""

        while True:
            try:
                log.debug("Listen to new data.")
                data = yield stream.read_bytes(50000, partial=True)
                metrics = data.split(b"\n")
                log.debug("New data: bytes={}, metrics={}".format(len(data), len(metrics)))
                if len(metrics) > 0:
                    metrics[0] = prev_read_metric + metrics[0]
                    for i in range(0, len(metrics) - 1):
                        self._ring_buffer.write(metrics[i])
                    prev_read_metric = metrics[len(metrics) - 1]
            except StreamClosedError as error:
                if error.real_error:
                    log.exception("Close stream. Read error: " + error.real_error)
                else:
                    log.debug("Close stream.")
                break
            except Exception:
                log.exception("Unexpected Error")
                break


class Sender:
    def send(self):
        raise NotImplementedError


class GraphiteSender(Sender):
    def __init__(self, sender_id, buffer, serializer, batch_size, server, port):
        self._sender_id = sender_id
        self._server = server
        self._port = port
        self._buffer = buffer
        self._awaken = False
        self._tcp_client = None
        self._serializer = serializer
        self._batch_size = batch_size

    @gen.coroutine
    def send(self):
        if self._awaken or not self._buffer.has_data_for(self._sender_id):
            log.debug("Already awaiken or no data.")
            return

        self._awaken = True
        stream = None
        try:
            log.debug("Try create TCPClient.")
            stream = yield TCPClient().connect(self._server, self._port)
            log.debug("TCPClient created.")
            while self._buffer.has_data_for(self._sender_id):
                data = self._buffer.scan(self._sender_id, self._batch_size)
                payload = self._serializer.serialize(data)
                log.debug("Write begin. MetricsCount: {}".format(len(data)))
                yield stream.write(payload)
                log.debug("Write completed.")
                self._buffer.shift_read_offset(self._sender_id, len(data))
            log.debug("TCPClient closing.")
            stream.close()
        except Exception:
            log.exception("Fail sand to graphite.")
            try:
                if stream:
                    stream.close()
            except Exception:
                pass
        finally:
            self._awaken = False


class SolomonHttpSender(Sender):
    BATCH_SIZE = 2000
    BODY_ENCODING = "ASCII"

    def __init__(self, sender_id, buffer, serializer, url, project, cluster, service, oauth_token, read_timeout, connect_timeout):
        self._sender_id = sender_id
        self._url = url
        self._buffer = buffer
        self._awaken = False
        self._http_client = None
        self._serializer = serializer
        self._read_timeout = read_timeout
        self._connect_timeout = connect_timeout
        self._oauth_token = oauth_token
        self._project = project
        self._cluster = cluster
        self._service = service

    def to_request(self, serialized_data):
        return HTTPRequest(
            "{}?project={}&cluster={}&service={}".format(self._url, self._project, self._cluster, self._service),
            "POST",
            headers = HTTPHeaders({"Content-Type": "application/json", "Authorization": "OAuth {}".format(self._oauth_token)}),
            body = bytes(json.dumps(serialized_data), SolomonHttpSender.BODY_ENCODING),
            connect_timeout = self._connect_timeout,
            request_timeout = self._read_timeout
        )

    @gen.coroutine
    def send(self):
        if self._awaken or not self._buffer.has_data_for(self._sender_id):
            return
        try:
            self._awaken = True
            self._http_client = httpclient.AsyncHTTPClient()
            while self._buffer.has_data_for(self._sender_id):
                data = self._buffer.scan(self._sender_id, SolomonHttpSender.BATCH_SIZE)
                serialized_data = self._serializer.serialize(data)
                request = self.to_request(serialized_data)
                log.debug("Ready to send data {}".format(len(data)))
                response = yield self._http_client.fetch(request)
                log.debug("Success send")
                self._buffer.shift_read_offset(self._sender_id, len(data))
        except httpclient.HTTPError as e:
            log.exception(
                "Bad solomon response status code. Server response: {} {}, {}".format(
                    e.response,
                    e.response.reason,
                    e.response.body
                )
            )
        except Exception:
            log.exception("Error during solomon send.")
        finally:
            self._awaken = False


class MetricDecoder:
    SPLITTER = b" "

    def __init__(self, metric_name_to_id=None, metric_id_to_name=None, next_metric_id=None):
        if metric_name_to_id:
            self._metric_name_to_id = metric_name_to_id
            self._metric_id_to_name = metric_id_to_name
            self._next_metric_id = next_metric_id
        else:
            self._metric_name_to_id = {}
            self._metric_id_to_name = {}
            self._next_metric_id = 0

    def decode(self, compacted_metric):
        res = bytearray(self._metric_id_to_name[compacted_metric[0]])
        res += MetricDecoder.SPLITTER
        res += compacted_metric[1]
        res += MetricDecoder.SPLITTER
        res += compacted_metric[2]
        return res

    def encode(self, metric):
        metric_name, value, timestamp = metric.split(MetricDecoder.SPLITTER)
        if metric_name not in self._metric_name_to_id:
            self._next_metric_id += 1
            self._metric_name_to_id[metric_name] = self._next_metric_id
            self._metric_id_to_name[self._next_metric_id] = metric_name
        return self._metric_name_to_id[metric_name], value, timestamp

    def shallow_copy(self):
        return MetricDecoder(self._metric_name_to_id.copy(), self._metric_id_to_name.copy(), self._next_metric_id)


class Serializer:
    def serialize(self, list):
        raise NotImplementedError


class GraphiteSerializer(Serializer):
    METRIC_SPLITTER = b"\n"
    METRIC_PARTS_SPLITTER = b" "
    METRIC_ENCODING = "utf-8"

    def __init__(self, prefix):
        self._prefix = bytearray(prefix, GraphiteSerializer.METRIC_ENCODING) if prefix else None

    def serialize(self, metrics):
        res = bytearray()
        for metric in metrics:
            if self._prefix:
                res += self._prefix
            metric_name, value, timestamp = metric.split(GraphiteSerializer.METRIC_PARTS_SPLITTER)
            timestamp = timestamp.split(b'.', 1)[0]
            timestamp = timestamp.split(b',', 1)[0]
            res += metric_name
            res += GraphiteSerializer.METRIC_PARTS_SPLITTER
            res += value
            res += GraphiteSerializer.METRIC_PARTS_SPLITTER
            res += timestamp
            res += GraphiteSerializer.METRIC_SPLITTER
        return res


class SolomonJsonSerializer():
    ENCODING = "ASCII"
    def __init__(self):
        self._common_labels = {
            "host": socket.getfqdn()
        }

    @staticmethod
    def to_sensor(metric):
        metric_str = str(metric, encoding=SolomonJsonSerializer.ENCODING)
        metric, value, timestamp = metric_str.split(" ")
        labels_list = metric.split(".", 11)
        labels = {}
        i = 0
        for label in labels_list:
            labels["l{}".format(i)] = label
            i += 1
        return {
            "labels": labels,
            "ts": float(timestamp),
            "value": value
        }

    def serialize(self, metrics):
        return {
            "commonLabels": self._common_labels,
            "sensors": [SolomonJsonSerializer.to_sensor(metric) for metric in metrics]
        }


class AlertStatus(Enum):
    OK = 0
    WARNING = 1
    CRITICAL = 2


class AlertResult(object):
    def __init__(self, name, status, message=""):
        self.name = name
        self.message = "" if message is None else message
        self.status = status

    def __str__(self):
        return "{};{}\n".format(self.status.value, self.message)


class HealthCheck:
    OK_MESSAGE = "No problem."

    def __init__(self, name):
        self._name = name
        self._alert_result = AlertResult(self._name, AlertStatus.OK, HealthCheck.OK_MESSAGE)

    def ok(self, message=OK_MESSAGE):
        self._alert_result = AlertResult(self._name, AlertStatus.OK, message)

    def error(self, message):
        self._alert_result = AlertResult(self._name, AlertStatus.CRITICAL, message)

    def warn(self, message):
        self._alert_result = AlertResult(self._name, AlertStatus.WARNING, message)

    def get_status(self):
        return self._alert_result


class HealthReporter:
    MAX_ALERT_MESSAGE_LEN = 1000

    def __init__(self, file):
        self._file = file
        self._health_checks = []

    def register_healthcheck(self, healthcheck):
        self._health_checks.append(healthcheck)

    def report_status(self):
        alert_results = [hc.get_status() for hc in self._health_checks]
        final_result = self.aggregate(alert_results)
        with open(self._file, 'w') as file:
            file.writelines((str(final_result),))

    def aggregate(self, alert_results):
        if len(alert_results) is 0:
            return AlertResult("Empty.", AlertStatus.OK)
        alert_results = sorted(alert_results, key=lambda x: x.status.value, reverse=True)
        aggregatd_status = alert_results[0].status
        message = ""
        for alert_result in alert_results:
            message += "{}:{}:{};".format(alert_result.status.name, alert_result.name, alert_result.message)
        return AlertResult(
            "aggregated",
            aggregatd_status,
            message[0: min(HealthReporter.MAX_ALERT_MESSAGE_LEN, len(message))]
        )


class BinaryFileStorage:
    def __init__(self, file):
        self._file = file

    def file_exists(self):
        my_file = Path(self._file)
        return my_file.is_file()

    def pickle(self, obj):
        with open(self._file, 'wb') as handle:
            log.debug("Start backup")
            pickle.dump(obj, handle, protocol=pickle.HIGHEST_PROTOCOL)
            log.debug("Finish backup")

    def unpickle(self):
        try:
            if self.file_exists():
                with open(self._file, 'rb') as handle:
                    return pickle.load(handle)
        except Exception:
            log.exception("Can't unpickle buffer and compactor.")
        return None


def _json_object_hook(d): return namedtuple('X', d.keys())(*d.values())


def read_settings(settings_file):
    return json.load(open(settings_file), object_hook=_json_object_hook)


def create_rotating_logging(level, max_bytes, backup_count):
    log.setLevel(level)
    fh = RotatingFileHandler(
        "/var/log/media_solomon_sender/solomon_sender.log",
        maxBytes=max_bytes,
        backupCount=backup_count
    )
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    fh.setFormatter(formatter)
    log.addHandler(fh)
    log.info("Log Configured")


def create_dir(file):
    try:
        abs_path = os.path.abspath(file)
        path = os.path.dirname(abs_path)
        Path(path).mkdir(parents=True)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise


def get_config_file():
    if os.path.exists("/etc/yandex-media-graphite-to-solomon-sender/solomon_sender_settings.json"):
        return "/etc/yandex-media-graphite-to-solomon-sender/solomon_sender_settings.json"
    else:
        return "/etc/yandex-media-graphite-to-solomon-sender/solomon_sender_settings-default.json"


def run_backup(buffer):
    shallow_copy = buffer.shallow_copy()
    executor.submit(lambda: backup_maker.pickle(shallow_copy))


def parse_args():
    desc = "Solomon and graphite sender. Without args uses default settings from config located" \
           "/etc/yandex-media-graphite-to-solomon-sender/solomon_sender_settings.json"
    argparser = argparse.ArgumentParser(description=desc)
    argparser.add_argument("-c", "--config", help="Override default config file.",
                           action="store", default=get_config_file())
    argumets = argparser.parse_args()
    return argumets


if __name__ == '__main__':
    args = parse_args()
    settings = read_settings(args.config)
    create_rotating_logging(
        settings.logging.level,
        settings.logging.file_size_bytes,
        settings.logging.backup_count
    )
    log.info("Used config: %s", args.config)
    backup_maker = BinaryFileStorage(settings.backup_settings.file)
    buffer = backup_maker.unpickle()
    if not buffer:
        decoder = MetricDecoder()
        healthcheck = HealthCheck("senders_health_check")
        buffer = CompactingSingleWriterMultipleReaderRingBuffer(
            settings.buffer.length,
            decoder,
            healthcheck
        )

    reader_ids_to_register = []
    if settings.solomon_http_sender.on:
        solomon_http_sender = SolomonHttpSender(
            settings.solomon_http_sender.sender_id,
            buffer,
            SolomonJsonSerializer(),
            settings.solomon_http_sender.url,
            settings.solomon_http_sender.project,
            settings.solomon_http_sender.cluster,
            settings.solomon_http_sender.service,
            settings.solomon_http_sender.oauth_token,
            settings.solomon_http_sender.read_timeout_millis,
            settings.solomon_http_sender.connect_timeout_millis
        )
        PeriodicCallback(
            solomon_http_sender.send,
            settings.solomon_http_sender.delay_millis
        ).start()
        reader_ids_to_register.append(settings.solomon_http_sender.sender_id)

    if settings.graphite_senders:
        for graphite_sender_settings in settings.graphite_senders:
            if graphite_sender_settings.on:
                graphite_sender = GraphiteSender(
                    graphite_sender_settings.sender_id,
                    buffer,
                    GraphiteSerializer(graphite_sender_settings.metric_prefix),
                    graphite_sender_settings.max_batch_size,
                    graphite_sender_settings.server,
                    graphite_sender_settings.port
                )
                PeriodicCallback(
                    graphite_sender.send,
                    graphite_sender_settings.delay_millis
                ).start()
                reader_ids_to_register.append(graphite_sender_settings.sender_id)

    buffer.register_readers(reader_ids_to_register)
    server = GraphiteReceiverServer(buffer)
    server.listen(settings.port)

    if settings.backup_settings.on:
        create_dir(settings.backup_settings.file)
        PeriodicCallback(
            lambda: run_backup(buffer),
            settings.backup_settings.delay_millis
        ).start()

    if settings.health_reporter.on:
        create_dir(settings.health_reporter.file)
        PeriodicCallback(
            buffer.check_health,
            settings.buffer.check_health_delay_millis
        ).start()
        health_reporter = HealthReporter(settings.health_reporter.file)
        health_reporter.register_healthcheck(buffer.get_healthcheck())
        PeriodicCallback(
            health_reporter.report_status,
            settings.health_reporter.delay_millis
        ).start()

    IOLoop.current().start()
