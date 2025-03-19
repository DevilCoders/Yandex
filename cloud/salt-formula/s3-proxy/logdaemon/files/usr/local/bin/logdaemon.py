#!/usr/bin/env python3
import argparse
import json
import glob
import math
import os
import re
import sys
import time
import yaml
import logging
from collections import OrderedDict
from stat import ST_DEV, ST_INO
from threading import Thread
from logging.handlers import WatchedFileHandler
try:
    from http.server import BaseHTTPRequestHandler, HTTPServer
except Exception:
    from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
try:
    from urllib.parse import urlparse
except Exception:
    from urlparse import urlparse


def load_config(path):
    try:
        return yaml.load(open(path), Loader=yaml.BaseLoader)
    except Exception as e:
        raise argparse.ArgumentTypeError(e)


# Logging setup
httpd_logger = logging.getLogger("httpd")
parser_logger = logging.getLogger("parser")
common_logger = logging.getLogger("common")


def setup_logging(logfile):
    fh = WatchedFileHandler(logfile)
    fh.setLevel(logging.INFO)
    formatter = logging.Formatter('%(asctime)s %(name)s\t%(levelname)s\t%(message)s')
    fh.setFormatter(formatter)

    httpd_logger.addHandler(fh)
    httpd_logger.setLevel(logging.INFO)

    parser_logger.addHandler(fh)
    parser_logger.setLevel(logging.INFO)

    common_logger.addHandler(fh)
    common_logger.setLevel(logging.INFO)


# HTTP GET handler
class S(BaseHTTPRequestHandler):

    def _set_headers_200(self):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()

    def _set_headers_404(self):
        self.send_response(404)
        self.send_header('Content-type', 'application/octet-stream')
        self.end_headers()

    def do_GET(self):
        query = urlparse(self.path).query
        if query:
            query_components = dict(qc.split('=') for qc in query.split('&'))
        else:
            query_components = {}
        rpath = urlparse(self.path).path
        if rpath == '/solomon':
            self._set_headers_200()
            self.wfile.write(Counter.__get_solomon_data__().encode())
        elif rpath == '/yasm':
            self._set_headers_200()
            self.wfile.write(Counter.__get_yasm_data__().encode())
        else:
            if query_components.get('service', False):
                self._set_headers_200()
                httpd_logger.info("Received parameters: {}".format(query_components))
                if query_components['service'] == 'yasm':
                    self.wfile.write(Counter.__get_yasm_data__().encode())
                elif query_components['service'] == 'solomon':
                    self.wfile.write(Counter.__get_solomon_data__().encode())
                else:
                    self.wfile.write("Please specify the service.".encode())
            else:
                self._set_headers_404()
                self.wfile.write("Please specify the service.".encode())

    def log_message(self, format, *args):
        httpd_logger.info("%s - - [%s] %s" %
                          (self.address_string(),
                           self.log_date_time_string(),
                           format % args))


def run_httpd(server_class=HTTPServer, handler_class=S, port=8088):
    httpd_logger.info("Starting httpd thread on {}:{}".format('127.0.0.1', port))
    server_address = ('127.0.0.1', port)
    httpd = server_class(server_address, handler_class)
    httpd.serve_forever()


# error.log handler
def shutdown(signum):
    sys.exit(0)


def histogram_bucket(value):
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
    return bucket


class Counter(object):
    yasm_storage = {}
    solomon_storage = {}

    def __init__(self):
        # self.storage = {}
        pass

    def count(self, msg, num):
        signal = self.yasm_storage.get(msg, 0)
        Counter.yasm_storage[msg] = signal + num

    def solomon_count(self, dic, num):
        mhash = json.dumps(dic)
        signal = self.solomon_storage.get(mhash, 0)
        Counter.solomon_storage[mhash] = signal + num

    def report(self):
        return Counter.storage

    @staticmethod
    def __get_yasm_data__():
        yasm_format = []
        for k in Counter.yasm_storage.keys():
            yasm_format.append(["s3mds_nginx_error_log_{}_dmmm".format(k),
                                Counter.yasm_storage[k]])

        return json.dumps(yasm_format)

    @staticmethod
    def __get_solomon_data__():
        solomon_format = []
        for k in Counter.solomon_storage.keys():
            solomon_format.append([json.loads(k),
                                   Counter.solomon_storage[k]])

        return json.dumps(solomon_format)


def error_log_handler(counter, line):

    if not line.strip():
        return

    exclude_list = ["limiting requests", "GET /ping HTTP"]

    try:
        for item in exclude_list:
            if item in line:
                return

        loglevel = re.search(r"\[(\w+)\]", line).groups()[0]
        counter.count(loglevel, 1)

    except Exception:
        parser_logger.error("Failed to parse line: {}".format(repr(line)))


def tskv_parse(line):
    return dict([k.split('=', 1) for k in line.split('\t')])


def access_service_log_handler(counter, line):

    # Example two records

    # timestamp=2019-05-16T06:27:20
    # request_id=1d3eb47ab00e61f4
    # request=authorize
    # status=Ok
    # error_desc=None
    # attempt=1
    # request_time=0.003

    # timestamp=2019-05-16T06:27:35
    # request_id=0c2df3e67884cf70
    # request=authenticate
    # status=Ok
    # error_desc=None
    # attempt=1
    # request_time=0.002

    if not line.strip():
        return

    logdict = tskv_parse(line)

    tags = OrderedDict()
    tags['name'] = 'request'
    tags['proxy_service'] = 'AccessService'
    tags['request_type'] = logdict['request']
    if logdict['status'] == 'Ok':
        tags['request_status_value'] = logdict['status']
        tags['request_status_desc'] = 'Ok'
    else:
        tags['request_status_value'] = 'Other'
        tags['request_status_desc'] = logdict['status']
    counter.solomon_count(tags, 1)

    tags = OrderedDict()
    tags['name'] = 'request_time'
    tags['proxy_service'] = 'AccessService'
    tags['timings_bucket'] = str(histogram_bucket(float(logdict['request_time'])))
    counter.solomon_count(tags, 1)

    # Empty metric for solomon graphs
    tags = OrderedDict()
    tags['name'] = 'request'
    tags['proxy_service'] = 'AccessService'
    tags['request_type'] = 'authenticate'
    tags['request_status_value'] = 'Ok'
    tags['request_status_desc'] = ''
    counter.solomon_count(tags, 0)
    tags['request_type'] = 'authorize'
    counter.solomon_count(tags, 0)
    tags['request_status_value'] = 'Other'
    counter.solomon_count(tags, 0)
    tags['request_type'] = 'authenticate'
    counter.solomon_count(tags, 0)


def log_parser(source_data_file, log_handler):
    try:
        source_data = open(source_data_file, 'r')
    except Exception as e:
        parser_logger.error("Can't open log file: {}".format(e))
        return
    source_data.seek(0, 2)
    stat = os.stat(source_data_file)
    dev, ino = stat[ST_DEV], stat[ST_INO]
    counter = Counter()

    parser_logger.info("Logparser thread started with handler: {} on file: {}".format(
        log_handler.__name__,
        source_data_file))

    while True:
        try:
            stat = os.stat(source_data_file)
        except Exception:
            continue
        changed = (stat[ST_DEV] != dev) or (stat[ST_INO] != ino)
        if not changed:
            new_data = True
            while new_data:
                line = source_data.readline()
                if line:
                    log_handler(counter, line)
                else:
                    new_data = False
            time.sleep(0.1)
        else:
            try:
                source_data.close()
                source_data = open(source_data_file, 'r')
                source_data.seek(0, 2)
                stat = os.stat(source_data_file)
                dev, ino = stat[ST_DEV], stat[ST_INO]
                parser_logger.info("File {} reopened".format(source_data_file))
                parser_logger.info(Counter.__get_yasm_data__())
            except Exception:
                pass


def make_threads(settings):
    new_thread = Thread(target=run_httpd,
                        name="httpd",
                        kwargs={})
    new_thread.daemon = True
    new_thread.start()

    for source in settings['config']['sources']:
        log_handler = settings['parsers'][settings['config'][source]['parser']]
        for logfile in settings['config'][source]['files']:
            new_thread = Thread(target=log_parser,
                                name=source,
                                kwargs={"source_data_file": logfile,
                                        "log_handler": log_handler})
            new_thread.start()


def create_daemon(settings):
    try:
        pid = os.fork()
    except OSError:
        raise Exception

    if pid == 0:
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
        common_logger.info('Find pattern for {}: {}'.format(log, glob.glob(log_files_pattern)))
        config[log]['files'] = glob.glob(log_files_pattern)
    return config


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Log parser and unistat daemon")
    parser.add_argument('-c', '--config', type=load_config, default='config.yaml')
    parser.add_argument('-f', '--force', action='store_false')
    parser.add_argument('-v', '--verbosity', default='INFO')

    # Add subparser for stop action on specified log file

    levels = {'INFO': logging.INFO,
              'DEBUG': logging.DEBUG}

    args = parser.parse_args()

    settings = {'parsers': {'error_log': error_log_handler,
                            'access_service_log': access_service_log_handler}}

    settings["pidfile"] = args.config['pidfile']
    settings["logfile"] = args.config['logfile']
    settings["overwritepid"] = args.force

    setup_logging(settings["logfile"])
    settings["config"] = expand_source_files(args.config)

    if settings['overwritepid']:
        if os.path.exists(settings["pidfile"]):
            common_logger.error("Pid file exists, exiting...")
            os._exit(1)
    else:
        common_logger.warning("Overwrite pid, because of 'force' flag received")

    create_daemon(settings)
