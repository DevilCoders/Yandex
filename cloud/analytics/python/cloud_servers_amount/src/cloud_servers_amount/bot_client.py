import re
import json
import requests
from urllib3.util.retry import Retry
from requests.adapters import HTTPAdapter
from collections import Counter
from clan_tools.logging.logger import default_log_config
import logging.config

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


class Req:
    conn_timeout = 5
    retries = 3
    bad_statuses = [400, 409, 422, 500, 502, 503, 504]
    response_timeout = None

    def _make_session(self):
        sess = requests.Session()
        retries = Retry(total=self.retries,
                        backoff_factor=self.conn_timeout,
                        status_forcelist=self.bad_statuses)
        sess.mount('http://', HTTPAdapter(max_retries=retries))
        sess.mount('https://', HTTPAdapter(max_retries=retries))
        return sess

    def make_request(self, url, method="GET", headers={}, data=None, params=None):
        if not headers:
            headers["Content-Type"] = "application/json"
        session = self._make_session()
        try:
            req = session.request(url=url, method=method, timeout=self.response_timeout, data=data, params=params, headers=headers)
        except requests.RequestException as err:
            print("{}".format(err))
            return ""
        if req.status_code not in [200, 201, 204]:
            print("I've got {}  HTTP Code for {} with data: {}, params: {}, headers: {}, text {}".format(req.status_code, url, data, params, headers, req.text))
        return req.text


class BotClient:
    def __init__(self, abc_filter=None):
        self.bot_api = "https://bot.yandex-team.ru/"
        self.abc_filter = re.compile("Yandex.Cloud") if (abc_filter is None) else abc_filter
        self.hosts_data = None

    def _get_bot_api_url(self, suffix):
        return self.bot_api + suffix

    def get_host_info(self, inv):
        request_data = {"inv": inv, "format": "json"}
        host_data = json.loads(Req().make_request(url=self._get_bot_api_url("api/consistof.php"), method="GET", params=request_data))['data']
        return host_data

    def normalize_host_info(self, host_info):
        phy_cpu_types = []
        phy_ram_types = []
        phy_gpu_types = []
        for info in host_info['Components']:
            if info['item_segment3'] == 'CPU':
                phy_cpu_types.append(info['attribute12'] + '; cores: ' + info['attribute27'])
            if info['item_segment2'] == 'GPU':
                phy_gpu_types.append(info['item_segment1'])
            if info['item_segment3'] == 'RAM':
                phy_ram_types.append(info['item_segment2'])
        for info in host_info['Connected']:
            if info['item_segment3'] == 'NODE-GPU':
                node_gpu_inv = info['instance_number']
                for add_info in self.get_host_info(node_gpu_inv)['Components']:
                    if add_info['item_segment2'] == 'GPU':
                        phy_gpu_types.append(add_info['item_segment1'])
        return {'cpu': Counter(phy_cpu_types), 'ram': Counter(phy_ram_types), 'gpu': Counter(phy_gpu_types)}

    def get_all_servers(self):
        request_data = {"name": "view_oops_hardware", "format": "json"}
        hosts_data = json.loads(Req().make_request(url=self._get_bot_api_url("api/view.php"), method="GET", params=request_data))
        return hosts_data

    @staticmethod
    def _get_abc_safe(server):
        try:
            server_abc = str(server['Service'])
        except:
            server_abc = server['Service']
        return server_abc

    def get_hosts_by_filter(self):
        logger.info('Downloading all hosts according to given filter...')
        self.hosts_data = [server for server in self.get_all_servers() if not (re.search(self.abc_filter, self._get_abc_safe(server)) is None)]

    def get_servers_info_by_filter(self):
        logger.info('Downloading all host info for all given hosts...')
        tmp_servers_info = []
        for server in self.hosts_data:
            server_inv = self._get_abc_safe(server)
            server_inv = server['Inv']
            server_fqdn = server['FQDN']
            server_dc = server['LocationSegment4']
            server_rack = server['LocationSegment5']

            server_info = self.get_host_info(server_inv)
            normal_server_info = self.normalize_host_info(server_info)
            for resource_type in normal_server_info.keys():
                for key, val in normal_server_info[resource_type].items():
                    tmp_servers_info.append({
                        'server_abc': server['Service'],
                        'server_inv': server_inv,
                        'server_fqdn': server_fqdn,
                        'server_dc': server_dc,
                        'server_rack': server_rack,
                        'resource_type': resource_type,
                        'type': key,
                        'count': val
                    })
        logger.info('Downloading completed')
        return tmp_servers_info
