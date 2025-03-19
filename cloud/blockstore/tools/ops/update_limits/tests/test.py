import json
import logging
import re
import pytest

from urllib.parse import urlparse, parse_qs
from http.server import BaseHTTPRequestHandler, HTTPServer
from subprocess import run
from threading import Thread

import yatest.common as common
import yatest.common.network as network


def create_http_handler(env):

    class Handler(BaseHTTPRequestHandler):

        def do_GET(self):
            if not self.path.startswith('/data-api/get/'):
                self.send_error(400)
                return

            body = None

            q = parse_qs(urlparse(self.path).query)
            logging.info(f'[HTTP] q: {q}')
            if 'FreeSpaceBytes|UsedSpaceBytes' in q.get('l.sensor', []):
                body = {
                    "sensors": [{
                        'labels': {
                            'sensor': 'FreeSpaceBytes'
                        },
                        'values': [
                            {"value": 3390005001656320},
                            {"value": 3492505001656320},
                            {"value": 0.1}]
                    },
                    {
                        'labels': {
                            'sensor': 'UsedSpaceBytes'
                        },
                        'values': [
                            {"value": 5073408606813000},
                            {"value": 5073408706813440},
                            {"value": 0.2}]
                    }]
                }

            if 'BytesCount' in q.get('l.sensor', []):
                sensor = {'labels': {'sensor': 'BytesCount'}}

                if 'ssd' in q.get('l.type', []):
                    sensor['values'] = [
                        {"value": 3254055723225000},
                        {"value": 3254059725225984},
                        {"value": 0.1}
                    ]

                if 'hdd' in q.get('l.type', []):
                    sensor['values'] = [
                        {"value": 2112957701881020},
                        {"value": 2112957701890048},
                        {"value": 0.2}
                    ]

                body = {"sensors": [sensor]}

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            if body is not None:
                self.wfile.write(json.dumps(body).encode('utf-8'))

        def do_POST(self):
            if self.path != '/pssh':
                self.send_error(400)
                return

            length = int(self.headers['content-length'])
            req = self.rfile.read(length).decode('utf-8')

            logging.info(f'[HTTP] length: {length}, req: {req}')

            data = json.loads(req)
            cmd = data.get('cmd')
            if cmd is None:
                self.send_error(400, "empty command")
                return

            host = data.get('host')
            if host is None:
                self.send_error(400, "empty host")
                return

            m = re.search(r'kikimr -s localhost:2135 db schema user-attribute get /([a-z]+)/NBS', cmd)
            if m is not None and m.group(1) == 'myt':
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()

                self.__pssh_write(stdout=[
                    '__volume_space_limit_hdd: 23091943206551552',
                    '__volume_space_limit_ssd: ' + str(env.ssd_limit),
                    '__volume_space_limit_ssd_nonrepl: 814606529121484',
                    '__volume_space_allocated: 12537079904804864',
                    '__volume_space_allocated_ssd: 3990998607192064',
                    '__volume_space_allocated_hdd: 2066085700378624',
                    '__volume_space_allocated_ssd_nonrepl: 491201450999808',
                    '__volume_space_allocated_ssd_system: 6661803513741312',
                ])

                return

            m = re.search(r'kikimr -s localhost:2135 db schema user-attribute set /([a-z]+)/NBS __volume_space_limit_ssd=([0-9]+)', cmd)
            if m is not None and m.group(1) == 'myt':
                env.ssd_limit = int(m.group(2))

                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()

                self.__pssh_write()

                return

            self.send_error(400, f"unknown command: {cmd}")

        def __pssh_write(self, stdout=[], stderr=[], return_code=0):
            self.wfile.write(json.dumps({
                "return_code": return_code,
                "stdout": stdout,
                "stderr": stderr,
            }).encode('utf-8'))

    return Handler


class Env:
    def __init__(self):
        self.__pm = network.PortManager()

        self.ssd_limit = 5457579995525788

    def update_limits(self, cluster, zone, dry_run=False):
        args = [
            common.binary_path("cloud/blockstore/tools/ops/update_limits/update_limits"),
            '--pssh-path', common.binary_path("cloud/blockstore/tools/testing/pssh-mock/pssh-mock"),
            '--solomon-url', f'http://localhost:{self.__port}/data-api/get/',
            '--cluster', cluster,
            '--zone', zone,
            '--yes',
            '-vvvv'
        ]

        if dry_run:
            args.append('--dry-run')

        return run(args, env={"PSSH_MOCK_PORT": str(self.__port)})

    def __enter__(self):
        self.__port = self.__pm.get_port()

        self.__httpd = HTTPServer(('localhost', self.__port), create_http_handler(self))
        self.__thread = Thread(target=self.__httpd.serve_forever, name='http-server')
        self.__thread.start()

        return self

    def __exit__(self, *args):
        if self.__httpd:
            self.__httpd.shutdown()

        if self.__thread:
            self.__thread.join()


@pytest.mark.parametrize("dry_run", [False, True])
def test_update_limits(dry_run):
    with Env() as env:
        expected_ssd_limit = env.ssd_limit if dry_run else 5632814514834199

        r = env.update_limits('prod', 'myt', dry_run=dry_run)

        if r.returncode != 0:
            pytest.fail(f'bad return code: {r.returncode}')

        if env.ssd_limit != expected_ssd_limit:
            pytest.fail(f'unexpected SSD limit: {env.ssd_limit}')
