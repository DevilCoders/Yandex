import os
import re
import json
import datetime
import asyncio
import requests
import time
import urllib.request
import pandas as pd
from urllib3.util.retry import Retry
from requests.adapters import HTTPAdapter
from concurrent.futures import ThreadPoolExecutor
import logging.config

from clan_tools.logging.logger import default_log_config
from clan_tools.data_adapters.YTAdapter import YTAdapter

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)

#  Первый блок - выгрузка кодом информации о серверах
#  Такую как:
#      server_abc
#      server_inv
#      server_fqdn
#      phy_cpu_types
#      phy_cpu_counts
#      virt_cpu_counts
#      phy_gpu_types
#      phy_gpu_counts


class Req:

    def __init__(self, conn_timeout=5, num_retries=3):
        self.conn_timeout = conn_timeout
        self.retries = num_retries
        self.bad_statuses = [400, 409, 422, 500, 502, 503, 504]
        self.response_timeout = None

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
            req = session.request(url=url,
                                  method=method,
                                  timeout=self.response_timeout,
                                  data=data, params=params,
                                  headers=headers)
        except requests.RequestException as err:
            logger.warning(err)
            return ""
        if req.status_code not in [200, 201, 204]:
            logger.info("I've got {}  HTTP Code for {} with data: {}, params: {}, headers: {}, text {}".format(
                req.status_code,
                url,
                data,
                params,
                headers,
                req.text))
        return req.text


class BotClient:

    def __init__(self):
        self.bot_api = "https://bot.yandex-team.ru/"
        self.abc_filter = re.compile("Yandex.Cloud")

    def _get_bot_api_url(self, suffix):
        return self.bot_api + suffix

    def get_host_info(self, inv):
        request_data = {"inv": inv, "format": "json"}
        response = Req().make_request(url=self._get_bot_api_url("api/consistof.php"), method="GET", params=request_data)
        try:
            host_data = json.loads(response)['data']
        except Exception as exc:
            logger.info(response)
            logger.error(exc)
            raise RuntimeError("{}".format(exc))
        return host_data

    def get_all_servers(self):
        request_data = {"name": "view_oops_hardware", "format": "json"}
        response = Req().make_request(url=self._get_bot_api_url("api/view.php"), method="GET", params=request_data)
        try:
            hosts_data = json.loads(response)
        except Exception as exc:
            logger.info(response)
            logger.error(exc)
            raise RuntimeError("{}".format(exc))
        return hosts_data

    def get_servers_info_by_filter(self):
        hosts_data = self.get_all_servers()
        tmp_servers_info = []
        for server in hosts_data:
            server_inv = server['Inv']
            server_fqdn = server['FQDN']
            try:
                server_abc = str(server['Service'])
            except:
                server_abc = server['Service']

            if (re.search(self.abc_filter, server_abc) is None):
                continue

            server_info = self.get_host_info(server_inv)
            phy_cpu_types = []
            phy_cpu_counts = []
            virt_cpu_counts = []
            virt_cpu_counts = []
            phy_gpu_types = []
            for info in server_info['Components']:
                if info['item_segment3'] == 'CPU':
                    phy_cpu_types.append(info['attribute12'])
                    phy_cpu_counts.append(info['attribute27'])
                    virt_cpu_counts.append(int(info['attribute27'])*2)
                if info['item_segment2'] == 'GPU':
                    phy_gpu_types.append(info['item_segment1'])
            tmp_servers_info.append({
                'server_abc': server['Service'],
                'server_inv': server_inv,
                'server_fqdn': server_fqdn,
                'phy_cpu_types': phy_cpu_types,
                'phy_cpu_counts': phy_cpu_counts,
                'virt_cpu_counts': virt_cpu_counts,
                'phy_gpu_types': phy_gpu_types,
                'phy_gpu_counts': len(phy_gpu_types)
            })
        return tmp_servers_info


def calculate_percentiles():

    res = BotClient().get_servers_info_by_filter()
    server_dict = dict()
    for server_info in res:
        server_dict[server_info["server_fqdn"]] = server_info
    logger.info(server_dict[server_info["server_fqdn"]])

    # https://wiki.yandex-team.ru/solomon/api/v2/#oauth
    # основные параметры для выгрузки процессов
    graphs = {
        "link": "http://solomon.yandex.net/api/v2/projects/yandexcloud/sensors/data?maxPoints=500&from={}T00%3A00%3A00Z&to={}T00%3A00%3A00Z",
        "headers": {"Content-Type": "text/plain", "Accept": "application/json", "Authorization": "OAuth {}".format(os.environ["SOLOMON_TOKEN"])},
        "data": {
            "cpu_usage": "{{cluster='cloud_prod', service='infra_hw', host='{}', metric='cpu_usage', type='application'}}",
            "memory_usage": "{{cluster='cloud_prod', service='infra_hw', host='{}', metric='memory_usage', type='application'}}",
        }
    }
    conductor_urls = [
        "https://c.yandex-team.ru/api/groups2hosts/cloud_prod_compute_vla",
        "https://c.yandex-team.ru/api/groups2hosts/cloud_prod_compute_sas",
        "https://c.yandex-team.ru/api/groups2hosts/cloud_prod_compute_myt"
    ]

    # выдает все сервера по url
    def getServers(url):
        servers = []
        req = urllib.request.Request(url)
        with urllib.request.urlopen(req) as f:
            for server in f:
                servers.append(str(server.strip(), 'utf-8'))
        return servers

    # все данные  по заданой data
    def getTimeSlice(url, data, headers):
        req = urllib.request.Request(url, data.encode(), headers)
        resp_code = 429
        retries_num = 5
        while ((resp_code == 429) | (resp_code == 504)) & (retries_num > 0):
            retries_num -= 1
            try:
                with urllib.request.urlopen(req) as f:
                    resp_code = f.code
                    response = f.read()
            except Exception as exc:
                logger.info("Catched Error")
                logger.warning(exc)
            if resp_code == 429:
                time.sleep(0.5)
                logger.warning("Catched Error 429")
            if resp_code == 504:
                time.sleep(0.5)
                logger.warning("Catched Error 504")
        try:
            return json.loads(response)
        except Exception as exc:
            logger.warning("Can not format to json {}".format(response))
            logger.warning("Ignored Error:\n{}".format(exc))
            return None

    # выдает все процессы удовлетворяющие данным параметрам
    def getDataFrame(server, date_from, date_to, graphs, signal):
        data = getTimeSlice(graphs["link"].format(date_from, date_to), graphs['data'][signal].format(server.split(".")[0]), graphs["headers"])
        processes_dict = {}
        for process_info in data['vector']:
            process_name = process_info['timeseries']['labels']['name']
            processes_dict[process_name] = process_info['timeseries']['values']
        return processes_dict

    # Функция проходится по всем серверам одного conductor_url за определенный день (По умолчанию сегодня)
    def create_table_by_urls(conductor_url, day):
        past = day - datetime.timedelta(days=1)
        today = day.isoformat()
        past = past.isoformat()

        yta = YTAdapter()

        final_df = None
        final_schema = None

        for signal in ['cpu_usage', 'memory_usage']:
            servers = getServers(conductor_url)

            for server in servers:
                processes = dict()

                usage = getDataFrame(server, past, today, graphs, signal)

                # Корректная компановка данных по блокам
                for k in usage:
                    if k[:3] == "epd" or k[:3] == "fhm" or k[:3] == "ef3":
                        continue
                    processes[k] = processes.get(k, []) + usage[k]

                for k in processes:
                    df = pd.Series(processes.get(k, []), dtype=float)
                    df = df.fillna(0)

                    # строка в YT table
                    write_df = pd.DataFrame({"server": [server],
                                             "server_abc": [server_dict[server]["server_abc"]],
                                             "phy_cpu_types": [server_dict[server]["phy_cpu_types"]],
                                             "phy_cpu_counts": [server_dict[server]["phy_cpu_counts"]],
                                             "virt_cpu_counts": [server_dict[server]["virt_cpu_counts"]],
                                             "phy_gpu_types": [server_dict[server]["phy_gpu_types"]],
                                             "phy_gpu_counts": [server_dict[server]["phy_gpu_counts"]],
                                             "conductor_url": [conductor_url],
                                             "process": [k],
                                             "signal": [signal],
                                             "date": [today]})

                    # schema для YT table
                    schema = [{'name': 'server', 'type': 'string', 'required': False},
                              {'name': 'server_abc', 'type': 'string', 'required': False},
                              {"name": "phy_cpu_types", 'type_v3': {"type_name": 'list', "item": {"type_name": "optional", "item": "string"}}},
                              {"name": "phy_cpu_counts", 'type_v3': {"type_name": 'list', "item": {"type_name": "optional", "item": "string"}}},
                              {"name": "virt_cpu_counts", 'type_v3': {"type_name": 'list', "item": {"type_name": "optional", "item": "int64"}}},
                              {"name": "phy_gpu_types", 'type_v3': {"type_name": 'list', "item": {"type_name": "optional", "item": "string"}}},
                              {'name': 'phy_gpu_counts', 'type': 'int64', 'required': False},
                              {'name': 'conductor_url', 'type': 'string', 'required': False},
                              {'name': 'process', 'type': 'string', 'required': False},
                              {'name': 'signal', 'type': 'string', 'required': False},
                              {'name': 'date', 'type': 'string', 'required': False}]

                    # Вывод квантилей по данным
                    # Замена % на q для более простой работы с данными в будущем
                    quant_dict = df.describe(percentiles=[.25, .50, .75, .80, .95, .99]).fillna(0).to_dict()
                    for key in quant_dict:
                        better_key = key
                        if key[-1] == "%":
                            better_key = "q" + key[:-1]
                        write_df[better_key] = quant_dict[key]
                        schema.append({"name": better_key, 'type': 'double', 'required': False})

                    if final_df is None:
                        final_df = write_df
                        final_schema = schema
                    else:
                        final_df = pd.concat((final_df, write_df), ignore_index=True)
        # Загрузка в YT
        yta.save_result("//home/cloud_analytics/import/system_services_consumption_additional_info/" + conductor_url[-3:] + "/" + past,
                        df=final_df, schema=final_schema, append=False, default_type='any')

    # Функция асинхронной выгрузки по разным conductor_urls
    async def create_tables_async(day=None):
        if day is None:
            day = datetime.date.today()
        with ThreadPoolExecutor(max_workers=4) as executor:
            loop = asyncio.get_event_loop()
            tasks = [
                loop.run_in_executor(
                    executor,
                    create_table_by_urls,
                    *(conductor_url, day)
                )
                for conductor_url in conductor_urls
            ]
            for response in await asyncio.gather(*tasks):
                pass

    # Асинхронная выгрузка данных
    loop = asyncio.get_event_loop()
    future = asyncio.ensure_future(create_tables_async())
    loop.run_until_complete(future)

    # json для корректной работы в нирване
    with open('output.json', 'w') as f:
        json.dump({
            "status": 'success'
        }, f)

if __name__ == '__main__':
    calculate_percentiles()
