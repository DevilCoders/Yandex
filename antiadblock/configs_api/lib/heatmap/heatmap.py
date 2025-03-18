# coding=utf-8
import logging
import time
import random
import socket
from collections import namedtuple
from multiprocessing.dummy import Pool as ThreadPool
from threading import Thread

import requests
from flask import Blueprint, jsonify

from antiadblock.configs_api.lib.auth.permissions import ensure_user_admin

from infra.yp_service_discovery.python.resolver.resolver import Resolver
from infra.yp_service_discovery.api import api_pb2

heatmap_api = Blueprint('heatmap_api', __name__)
heatmap_api.heatmap_data = None

HostInfo = namedtuple('HostInfo', ['host', 'dc', 'resource_type', 'ipv6'])

POD_SET_IDS_BY_DC = {
    'man': ['cryprox-man'],
    'vla': ['cryprox-vla'],
    'sas': ['cryprox-sas'],
    'iva': ['cryprox-iva'],
    'myt': ['cryprox-myt']
}
HEATMAP = None


class Heatmap:
    def __init__(self, hosts_info):
        self.hosts_info = hosts_info
        thread_pool_size = min(128, len(self.hosts_info))
        logging.info("starting pool with size = {}".format(thread_pool_size))
        self.tp = ThreadPool(processes=thread_pool_size)

    def set_hosts_info(self, hosts_info):
        self.hosts_info = hosts_info

    def request_heatmap(self):
        def load_metrics(host_info):
            data = {}

            def req(timeout):
                host = '[{}]'.format(host_info.ipv6) if host_info.ipv6 else host_info.host
                return requests.get('http://{}:8080/system_metrics'.format(host), timeout=timeout).json()

            for to in (0.15, 0.2):
                try:
                    data = req(to)
                    break
                except Exception:
                    pass  # it ok to fail request
            data.update(host_info._asdict())
            return data

        return self.tp.map(load_metrics, self.hosts_info)


def __discover_yp_hosts():
    result = list()
    resolver = Resolver(client_name='aab-heatmap:{}'.format(socket.gethostname()), timeout=5)
    request = api_pb2.TReqResolveEndpoints()
    for cluster, set_ids in POD_SET_IDS_BY_DC.items():
        request.cluster_name = cluster
        for set_id in set_ids:
            request.endpoint_set_id = set_id
            sd_response = resolver.resolve_endpoints(request)
            for endpoint in sd_response.endpoint_set.endpoints:
                result.append(HostInfo(host=endpoint.fqdn, dc=cluster, resource_type='yp', ipv6=endpoint.ip6_address))
    return result


def __run_heatmap_loop():
    global HEATMAP
    while True:
        try:
            HEATMAP = Heatmap(__discover_yp_hosts())
            break
        except Exception:
            logging.exception("failed to discover hosts, will try again")
            time.sleep(1)
    hosts_info_reinit_loop_thread = Thread(target=__reinit_heatmap_hosts_info)
    hosts_info_reinit_loop_thread.daemon = True
    hosts_info_reinit_loop_thread.start()
    while True:
        try:
            start = time.time()
            heatmap_api.heatmap_data = {
                'ts': int(time.time()),
                'heatmap': HEATMAP.request_heatmap()
            }
            sleep_time = max(0.2, 1 - time.time() + start)
            logging.info("end heatmap cycle, sleeping for {}".format(sleep_time))
            time.sleep(sleep_time)
        except Exception:
            logging.exception("heatmap loop failed")
            time.sleep(1)


def __reinit_heatmap_hosts_info():
    while True:
        time.sleep(300 + 100 * random.random())
        try:
            HEATMAP.set_hosts_info(__discover_yp_hosts())
        except Exception:
            logging.exception("failed to discover hosts, while reiniting heatmap")


__heatmap_loop_thread = Thread(target=__run_heatmap_loop)
__heatmap_loop_thread.daemon = True
__heatmap_loop_thread.start()


@heatmap_api.route('/get_heatmap', methods=['GET'])
def get_heatmap():
    ensure_user_admin()
    if heatmap_api.heatmap_data is None:
        return '', 204
    return jsonify({'result': heatmap_api.heatmap_data})
