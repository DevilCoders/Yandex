#!/usr/bin/env python3
import argparse
import glob
import json
import os
import re
import sys
import yaml
import time
import math
import setproctitle
from datetime import datetime

import smtplib
from email.message import EmailMessage

from stat import ST_DEV, ST_INO
import threading
from threading import Thread
try:
    from http.server import BaseHTTPRequestHandler, HTTPServer
except Exception:
    from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import logging
from logging.handlers import WatchedFileHandler


def load_config(path):
    try:
        return yaml.load(open(path), Loader=yaml.BaseLoader)
    except Exception as e:
        raise argparse.ArgumentTypeError(e)


def tskv_parse(line):
    return dict([k.split('=', 1) for k in line[5:].split('\t')])


def validate_time(value):
    if value > 50000000:
        return 50000000
    elif value < -50000000:
        return -50000000
    return value


# Logging setup
httpd_logger = logging.getLogger("httpd")
parser_logger = logging.getLogger("parser")
common_logger = logging.getLogger("common")


alert_recipients = ['agodin@yandex-team.ru',
                    'hotosho@yandex-team.ru',
                    'kanst9@yandex-team.ru',
                    'shindo@yandex-team.ru']


def setup_logging(logfile):
    fh = WatchedFileHandler(logfile)
    fh.setLevel(logging.INFO)
    formatter = logging.Formatter(
        '%(asctime)s %(name)s\t%(levelname)s\t%(message)s')
    fh.setFormatter(formatter)

    httpd_logger.addHandler(fh)
    httpd_logger.setLevel(logging.INFO)

    parser_logger.addHandler(fh)
    parser_logger.setLevel(logging.INFO)

    common_logger.addHandler(fh)
    common_logger.setLevel(logging.INFO)


class BucketStore(object):
    """Cacheable storage of bucket names"""

    def __init__(self, buckets_file, update_period_sec):
        self.buckets_file = buckets_file
        self.update_period_sec = update_period_sec

        self._storage = []
        self._update_timestamp = 0

    def update_bucket_cache(self):
        try:
            storage = []
            with open(self.buckets_file) as f:
                for name in f.readlines():
                    storage.append(name.strip())
            self._storage = storage
            self._update_timestamp = int(time.time())
            parser_logger.info("bucket cache updated with {} buckets".format(len(self._storage)))
        except Exception as e:
            parser_logger.error("failed while update bucket cache: {}".format(e))

    def aggregate(self, name):
        if name.startswith("internal-dbaas"):
            name = "internal-dbaas"
        elif name.startswith("yandexcloud-dbaas"):
            name = "yandexcloud-dbaas"
        elif name.startswith("cloud-storage"):
            name = "cloud-storage"
        elif name.startswith("buttload"):
            name = "buttload"
        return name

    def validate_bucket(self, name):
        if int(time.time()) - self._update_timestamp > self.update_period_sec:
            self.update_bucket_cache()

        if name not in self._storage:
            name = "unknown"

        return self.aggregate(name)


# HTTP GET handler
class S(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'application/octet-stream')
        self.end_headers()

    def do_GET(self):
        self._set_headers()
        try:
            self.wfile.write(Counter.__get_yasm_data__().encode())
        except Exception as e:
            httpd_logger.error("while handle GET request '{}'".format(e))

    def log_message(self, format, *args):
        httpd_logger.info(
            "%s - - [%s] %s" % (self.address_string(),
                                self.log_date_time_string(), format % args))


def run_httpd(server_class=HTTPServer, handler_class=S, port=8088):
    setproctitle.setproctitle(threading.currentThread().name)
    httpd_logger.info("starting httpd...")
    server_address = ('127.0.0.1', port)
    httpd = server_class(server_address, handler_class)
    httpd.serve_forever()


# error.log handler
def shutdown(signum):
    sys.exit(0)


def histogram_bucket(value):
    if value == 0:
        return 0

    log_base = 1.5
    min_log = -50
    max_log = 50
    borders = {}
    for i in range(-50, 50):
        borders[i+50+2] = 1.5 ** i

    offset = 0
    if value is None:
        pass
    else:
        offset = math.floor(max(min_log - 1,
                                min(max_log,
                                    math.log(value)/math.log(log_base)))
                            - min_log + 1) + 1

        bucket = math.floor(borders[offset] * 1000 + 0.5)
    return bucket/1000


class Counter(object):
    storage = {}
    storage_hgram = {}

    broken_keys_storage = {}

    def __init__(self):
        # self.storage = {}
        pass

    @staticmethod
    def broken_keys_exists():
        if len(Counter.broken_keys_storage) > 0:
            return True
        return False

    @staticmethod
    def broken_keys():
        broken_keys_dict = Counter.broken_keys_storage
        Counter.broken_keys_storage = {}
        return broken_keys_dict

    def broken_key_add(self, bucket, key, request_time, request_id):
        bucket_info = Counter.broken_keys_storage.setdefault(
            bucket, {})
        key_info = bucket_info.setdefault(key, {"request_id": [],
                                                "request_nums": 0,
                                                "time_first": None,
                                                "time_last": None})
        key_info["request_id"].append(request_id)
        key_info["request_nums"] += 1
        # r_time = datetime.strptime(request_time, '%Y-%m-%d %H:%M:%S.%f')
        r_time = datetime.strptime(request_time, '%Y-%m-%dT%H:%M:%S.%f%z')
        if key_info["time_first"] is None or key_info["time_first"] > r_time:
            key_info["time_first"] = r_time
        elif key_info["time_last"] is None or key_info["time_last"] < r_time:
            key_info["time_last"] = r_time

        parser_logger.info("added broken key: '{}', '{}', '{}'".format(bucket, key, request_id))
        Counter.broken_keys_storage[bucket] = bucket_info

    def count(self, msg, num):
        signal = self.storage.get(msg, 0)
        Counter.storage[msg] = signal + num

    def hgram(self, msg, num):
        hgram = self.storage_hgram.get(msg, {})
        time_bucket = histogram_bucket(num)
        bucket_weight = hgram.get(time_bucket, 0)
        hgram[time_bucket] = bucket_weight + 1
        Counter.storage_hgram[msg] = hgram

    def report(self):
        return Counter.storage

    @staticmethod
    def __get_yasm_data__():
        yasm_format = []
        for k in Counter.storage.keys():
            yasm_format.append([
                k, Counter.storage[k]
            ])

        for k in Counter.storage_hgram.keys():
            histogram = [list(pair) for pair in Counter.storage_hgram[k].items()]
            histogram.sort(key=lambda x: x[0])
            yasm_format.append([
                k, histogram
            ])
        Counter.storage_hgram = {}

        return json.dumps(yasm_format)


def send_mail(from_addr, to_addr, subject, message):

    msg = EmailMessage()
    msg.set_content(message)
    msg['Subject'] = subject
    msg['From'] = from_addr
    msg['To'] = to_addr

    s = smtplib.SMTP('localhost')
    s.send_message(msg)
    s.quit()


def mailer(settings):
    setproctitle.setproctitle(threading.currentThread().name)
    hostname = os.uname()[1]
    from_addr = "root@{}".format(hostname)
    to_addr = settings["config"]["broken_keys"].get("mailto")
    subject = "BROKEN KEYS FOUND"

    parser_logger.info("starting mailer...")
    # broken_keys = {
    #     "hotosho": {"key1": {'request_id': ['93862508f1913b5e', 'ca231332091a0e8b'],
    #                          'request_nums': 10,
    #                          'time_first': '2019-10-13 15:01:29.520953',
    #                          'time_last': '2019-10-13 15:01:29.522863'},
    #                 "key2": {'request_id': ['b2270ab9d91a5f45', '85cb91b2a52d743c'],
    #                          'request_nums': 9,
    #                          'time_first': '2019-10-13 11:01:29.520953',
    #                          'time_last': '2019-10-13 16:01:29.522863'}}
    # }

    while True:
        time.sleep(60)
        if Counter.broken_keys_exists():
            keys_report = ""
            broken_keys = Counter.broken_keys()
            for bucket in broken_keys:
                keys = broken_keys[bucket]
                for key in keys:
                    request_nums = keys[key].get("request_nums")
                    time_range = "{} - {}".format(keys[key].get("time_first"),
                                                  keys[key].get("time_last"))
                request_id = ", ".join(keys[key].get("request_id")[:3])
                bucket_template = """
      bucket: {}
      key: {}
      failed_requests: {}
      request_time: {}
      request_id: {}

                """.format(bucket, key, request_nums, time_range, request_id)

                keys_report += bucket_template
            message = """
    Information for {}:
            {}
            """.format(hostname, keys_report)
            parser_logger.info(
                "sending email about broken keys for buckets: {}".
                format(", ".join(broken_keys.keys())))

            send_mail(from_addr,
                      to_addr,
                      subject,
                      message)


def error_log_handler(counter, line, settings):
    setproctitle.setproctitle(threading.currentThread().name)
    if not line.strip():
        return

    exclude_list = [
        "GET /ping HTTP",
        "GET /s3-ping HTTP",
        "limiting requests",
        "server: nonexisting",
        "tcpinfo value is odd",
        "solomon-backup",
        "namespaces in /var/tmp/mm.namespaces",
        "S3 buckets in /var/cache/yasm/buckets_names",
        "/etc/nginx/html/stats/services",
        "/etc/nginx/html/stats/buckets"
    ]

    try:
        for item in exclude_list:
            if item in line:
                return

        loglevel = re.search(r"\[(\w+)\]", line).groups()[0]
        counter.count("s3mds_nginx_error_log_{}_dmmm".format(loglevel), 1)

    except Exception:
        parser_logger.error("failed to parse line: {}".format(line))


def broken_key_handler(counter, line, settings):
    setproctitle.setproctitle(threading.currentThread().name)
    try:
        try:
            data = json.loads(line)
        except Exception:
            parser_logger.error("broken_key_handler: failed to load line '{}'".format(line))

        if data.get("status", None) == "file_not_found":
            request_time = data.get("ts", datetime.now().strftime('%Y-%m-%dT%H:%M:%S.%f+03:00'))
            request_id = data.get("request_id", "-")
            bucket = data.get("bucket", "-")
            name = data.get("key", "-")
            counter.broken_key_add(bucket, name, request_time, request_id)
    except Exception as e:
        parser_logger.error("broken_key_handler: failed with error '{}'".format(e))


def status_code_to_string(code):
    status = int(code)
    if 200 <= status < 300:
        status = '2xx'
    elif 300 <= status < 400:
        status = '3xx'
    elif 400 <= status < 500:
        if status == 404:
            status = '404'
        elif status == 429:
            status = '429'
        elif status == 499:
            status = '499'
        else:
            status = '4xx'
    elif 500 <= status:
        if status == 501:
            status = '501'
        else:
            status = '5xx'
    else:
        status = 'unknown'

    return status


bucket_cache = BucketStore("/var/cache/yasm/buckets_names", 600)


def s3_goose_access_handler(counter, line, settings):
    setproctitle.setproctitle(threading.currentThread().name)
    try:
        try:
            data = json.loads(line)
        except Exception:
            counter.count("prj={0};tier={0};s3mds_goose_access-failed_to_parse_dmmm"
                          "".format("none"), 1)
            return

        status_code = data.get("status", False)
        if status_code:
            status = status_code_to_string(status_code)
        else:
            status = 'unknown'

        method = data.get("request_method")
        bucket = bucket_cache.validate_bucket(data.get("bucket", "unknown"))
        owner = data.get("owner", "none")
        if owner == "":
            owner = "none"
        bytes_received = int(data.get("content_length", 0))
        bytes_sent = int(data.get("response_length", 0))

        mtype = {'GET': 'read',
                 'HEAD': 'read',
                 'PUT': 'modify',
                 'POST': 'modify',
                 'DELETE': 'modify'}
        method_type = mtype.get(method)

        counter.count("prj={};tier={};s3mds_goose_access-method_{}_{}_dmmm".format(
            bucket, owner, method, status), 1)
        counter.count("prj={};tier={};s3mds_goose_access-status_{}_dmmm".format(
            bucket, owner, status), 1)
        counter.count("prj={};tier={};s3mds_goose_access-bytes_recv_dmmm".format(bucket, owner),
                      bytes_received)
        counter.count("prj={};tier={};s3mds_goose_access-bytes_sent_dmmm".format(bucket, owner),
                      bytes_sent)
        counter.count("prj={};tier={};s3mds_goose_access-bytes_recv_dmmx".format(bucket, owner),
                      bytes_received)
        counter.count("prj={};tier={};s3mds_goose_access-bytes_sent_dmmx".format(bucket, owner),
                      bytes_sent)

        # timings
        request_time = data.get("request_time", "UNKNOWN")
        if request_time == "UNKNOWN":
            counter.count("prj={};tier={};s3mds_goose_access-failed_timings_dmmm".format(
                bucket, owner), 1)
        else:
            request_time = float(request_time)
            if bytes_received < 5*1024*1024 \
               and bytes_sent < 5*1024*1024:
                counter.hgram(
                    "prj={};tier={};s3mds_goose_access-{}_timings_ahhh".format(
                        bucket, owner, method_type),
                    request_time)

    except Exception as e:
        counter.count("prj={0};tier={0};s3mds_goose_access-failed_to_parse_dmmm"
                      "".format("none"), 1)
        parser_logger.error("s3-goose-access failed with: {}".format(e))


def s3_goose_db_handler(counter, line, settings):
    setproctitle.setproctitle(threading.currentThread().name)
    try:
        try:
            data = json.loads(line)
        except Exception:
            counter.count("prj={0};tier={0};s3mds_goose_db-fail_to_parse_dmmm".format("none"), 1)
            return

        if data.get("msg", "") == 'QueryContext':
            return

        bucket = bucket_cache.validate_bucket(data.get("bucket", "unknown"))
        owner = data.get("owner", "none")
        db_attempt = data.get("db_attempt", "unknown")
        request_level = data.get("level")

        counter.count("prj={};tier={};s3mds_goose_db-request_dmmm".format(
            bucket, owner), 1)
        counter.count("prj={};tier={};s3mds_goose_db-attempt_{}_dmmm".format(
            bucket, owner, db_attempt), 1)
        counter.count("prj={};tier={};s3mds_goose_db-level_{}_dmmm".format(
            bucket, owner, request_level), 1)

        # timings
        request_time = data.get("db_duration", "UNKNOWN")
        if request_time == "UNKNOWN":
            counter.count("prj={};tier={};s3mds_goose_db-failed_timings_dmmm".format(
                bucket, owner), 1)
        else:
            request_time = float(request_time)
            counter.hgram("prj={};tier={};s3mds_goose_db-timings_ahhh".format(
                bucket, owner), request_time)

    except Exception as e:
        counter.count("prj={0};tier={0};s3mds_goose_db-fail_to_parse_dmmm".format("none"), 1)
        parser_logger.error("s3-goose-pg failed with: {}".format(e))


def s3_goose_sysdb_handler(counter, line, settings):
    setproctitle.setproctitle(threading.currentThread().name)
    try:
        try:
            data = json.loads(line)
        except Exception:
            counter.count("prj={0};tier={0};s3mds_goose_sysdb-fail_to_parse_dmmm".format("none"),
                          1)
            return
        args = data.get("args", [])
        if len(args) == 0:
            bucket_from_args = "unknown"
        else:
            bucket_from_args = args[0]
        bucket = bucket_cache.validate_bucket(bucket_from_args)
        owner = data.get("owner", "none")
        request_level = data.get("level")

        counter.count("prj={};tier={};s3mds_goose_sysdb-request_dmmm".format(
            bucket, owner), 1)
        counter.count("prj={};tier={};s3mds_goose_sysdb-level_{}_dmmm".format(
            bucket, owner, request_level), 1)

        # timings
        request_time = data.get("time", "UNKNOWN")
        if request_time == "UNKNOWN":
            counter.count("prj={};tier={};s3mds_goose_sysdb-failed_timings_dmmm".format(
                bucket, owner), 1)
        else:
            request_time = float(request_time)
            counter.hgram("prj={};tier={};s3mds_goose_sysdb-timings_ahhh".format(
                bucket, owner), request_time)

    except Exception as e:
        counter.count("prj={0};tier={0};s3mds_goose_sysdb-fail_to_parse_dmmm".format("none"), 1)
        parser_logger.error("s3-goose-pg failed with: {}".format(e))


def s3_goose_mds_handler(counter, line, settings):
    setproctitle.setproctitle(threading.currentThread().name)
    try:
        try:
            data = tskv_parse(line)
        except Exception as e:
            parser_logger.error("s3-goose-mds failed with tskv_parse: {}, line: {}".format(e, line))
            return

        counter.count("s3mds_goose_mds-common_rps_dmmm", 1)

        access = data.get("access")
        if access == 'transaction':
            status = data.get("status", 0)
            cmd = data.get("cmd")
            bytes_received = int(data.get("request_size", 0))
            bytes_sent = int(data.get("replies_size", 0))

            recv_queue_time = validate_time(float(data.get("recv_queue_time", 0)))/1000
            recv_time = validate_time(float(data.get("recv_time", 0)))/1000
            send_queue_time = validate_time(float(data.get("send_queue_time", 0)))/1000
            send_time = validate_time(float(data.get("send_time", 0)))/1000
            total_time = validate_time(float(data.get("total_time", 0)))/1000

            counter.count("s3mds_goose_mds-tr-cmd_{}-{}_dmmm".format(cmd, status), 1)
            # timings
            if bytes_received < 5*1024*1024 and bytes_sent < 5*1024*1024:
                counter.hgram("s3mds_goose_mds-tr-recv_queue_time_timings_ahhh", recv_queue_time)
                counter.hgram("s3mds_goose_mds-tr-recv_time_timings_ahhh", recv_time)
                counter.hgram("s3mds_goose_mds-tr-send_queue_time_timings_ahhh", send_queue_time)
                counter.hgram("s3mds_goose_mds-tr-send_time_timings_ahhh", send_time)
                counter.hgram("s3mds_goose_mds-tr-total_time_timings_ahhh", total_time)

        elif access == 'client':
            cmd = data.get("cmd")
            bytes_received = int(data.get("request_size", 0))
            bytes_sent = int(data.get("read_data_size", 0)) + int(data.get("read_json_size", 0))

            total_time = validate_time(float(data.get("total_time", 0)))/1000

            counter.count("s3mds_goose_mds-cl-cmd_{}_dmmm".format(cmd), 1)
            # timings
            if bytes_received < 5*1024*1024 and bytes_sent < 5*1024*1024:
                counter.hgram("s3mds_goose_mds-cl-total_time_timings_ahhh", total_time)

        else:
            counter.count("s3mds_goose_mds-other_rps_dmmm", 1)

    except Exception as e:
        parser_logger.error("s3-goose-mds failed with counters: {}, line: {}".format(e, line))


def s3_goose_cache_handler(counter, line, settings):
    setproctitle.setproctitle(threading.currentThread().name)
    try:
        try:
            data = json.loads(line)
        except Exception:
            counter.count("prj={0};tier={0};s3mds_goose_cache-fail_to_parse_dmmm".format("none"),
                          1)
            return
        bucket = bucket_cache.validate_bucket(data.get("bucket", "unknown"))
        owner = data.get("owner", "none")
        status = data.get("status", "unknown")

        counter.count("prj={};tier={};s3mds_goose_cache-status_{}_dmmm".format(
            bucket, owner, status), 1)
        counter.count("prj={};tier={};s3mds_goose_cache-status_all_dmmm".format(
            bucket, owner), 1)

        # timings
        request_time = data.get("request_time", "UNKNOWN")
        if request_time == "UNKNOWN":
            counter.count("prj={};tier={};s3mds_goose_cache-failed_timings_dmmm".format(
                bucket, owner), 1)
        else:
            request_time = float(request_time)
            counter.hgram("prj={};tier={};s3mds_goose_cache-timings_ahhh".format(
                bucket, owner), request_time)
    except Exception as e:
        counter.count("prj={0};tier={0};s3mds_goose_cache-fail_to_parse_dmmm".format("none"), 1)
        parser_logger.error("s3-goose-cache failed with: {}".format(e))


def log_parser(source_data_file, log_handler, settings):
    try:
        while True:
            try:
                source_data = open(source_data_file, 'r')
                break
            except Exception as e:
                parser_logger.error("can't open log file: {}".format(e))
                time.sleep(1)
        source_data.seek(0, 2)
        stat = os.stat(source_data_file)
        dev, ino = stat[ST_DEV], stat[ST_INO]
        counter = Counter()

        parser_logger.info(
            "logparser started on file: {}".format(source_data_file))

        while True:
            try:
                stat = os.stat(source_data_file)
            except Exception:
                continue
            changed = (stat[ST_DEV] != dev) or (stat[ST_INO] != ino)
            if not changed:
                new_data = True
                while new_data:
                    try:
                        lines = source_data.readline()
                        if not lines:
                            new_data = False
                        else:
                            while not lines.endswith('\n'):
                                lines += source_data.readline()
                            for line in lines.splitlines():
                                log_handler(counter, line, settings)
                    except Exception as e:
                        parser_logger.error("log_parser '{}' while read line from '{}'"
                                            ": '{}'".format(
                                                log_handler.__name__, source_data_file, e))
                time.sleep(0.1)
            else:
                try:
                    source_data.close()
                    source_data = open(source_data_file, 'r')
                    source_data.seek(0, 2)
                    stat = os.stat(source_data_file)
                    dev, ino = stat[ST_DEV], stat[ST_INO]
                    parser_logger.info("file {} reopened".format(source_data_file))
                except Exception:
                    pass

    except Exception as e:
        parser_logger.error("log_parser exception: '{}'".format(e))


def make_threads(settings):
    thread_1 = Thread(target=run_httpd, name="logd_httpd", kwargs={})
    thread_1.daemon = True
    thread_1.start()

    thread_2 = Thread(target=mailer, name="logd_mailer", kwargs={"settings": settings})
    thread_2.daemon = True
    thread_2.start()

    for source in settings["config"]["sources"]:
        log_handler = settings['parsers'][settings['config'][source]['parser']]
        for logfile in settings['config'][source]['files']:
            new_thread = Thread(target=log_parser,
                                name="logd_" + source,
                                kwargs={
                                    "source_data_file": logfile,
                                    "log_handler": log_handler,
                                    "settings": settings
                                })
            new_thread.start()


def create_daemon(settings):
    try:
        pid = os.fork()
    except OSError:
        raise Exception

    if pid == 0:
        setproctitle.setproctitle("logd_main")
        os.setsid()
        os.dup2(0, 1)
        os.dup2(0, 2)
        # sys.stdout = log_file
        # sys.stderr = log_file
        with open(settings["pidfile"], 'w') as pid_file:
            pid_file.write("{}".format(os.getpid()))
        common_logger.info("pid: {}".format(os.getpid()))
        make_threads(settings)
    else:
        os._exit(0)


def expand_source_files(config):
    for log in config.get('sources'):
        log_files_pattern = config.get(log)['path']
        common_logger.info('find pattern for {}: {}'.format(
            log, glob.glob(log_files_pattern)))
        config[log]['files'] = glob.glob(log_files_pattern)
    return config


def process_exists(pid):
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    else:
        return True


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Log parser and unistat daemon")
    parser.add_argument('-c',
                        '--config',
                        type=load_config,
                        default='config.yaml')
    parser.add_argument('-v', '--verbosity', default='INFO')

    levels = {'INFO': logging.INFO, 'DEBUG': logging.DEBUG}

    args = parser.parse_args()

    settings = {}
    settings["pidfile"] = args.config["pidfile"]
    settings["logfile"] = args.config["logfile"]

    settings["parsers"] = {
        "error_log": error_log_handler,
        "broken_key": broken_key_handler,
        "s3_goose_access": s3_goose_access_handler,
        "s3_goose_mds": s3_goose_mds_handler,
        "s3_goose_db": s3_goose_db_handler,
        "s3_goose_sysdb": s3_goose_sysdb_handler,
        "s3_goose_cache": s3_goose_cache_handler
    }

    settings["config"] = expand_source_files(args.config)

    setup_logging(settings["logfile"])

    if os.path.exists(settings["pidfile"]):
        with open(settings["pidfile"], "r") as pfile:
            proc_pid = int(pfile.read())
        if process_exists(proc_pid):
            common_logger.error("running process exists, exiting...")
            os._exit(1)
        else:
            common_logger.info(
                "there is no running process, overwriting pid file")

    create_daemon(settings)
