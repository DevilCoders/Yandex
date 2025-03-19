import re
import requests

from cloud.blockstore.pylibs import common


class SolomonClient:

    _SOLOMON_PROJECT = 'nbs'
    _SOLOMON_USER = 'robot-yc-nbs'
    _SOLOMON_URL = 'http://solomon.yandex.net/data-api/get'

    class Error(Exception):
        pass

    def __init__(self, logger):
        self._logger = logger

    def get_current_nbs_version(self, cluster: str, host: str) -> str:
        host = re.search('(.*).cloud', host).group(1)
        url = (f'{self._SOLOMON_URL}?project={self._SOLOMON_PROJECT}&who={self._SOLOMON_USER}'
               f'&cluster={cluster}&service=server&l.host={host}&l.sensor=version&b=1m')

        @common.retry(tries=3, delay=10, exception=requests.HTTPError)
        def _fetch_sensors(url):
            r = requests.get(url)
            r.raise_for_status()
            return r.json()['sensors']

        sensors = _fetch_sensors(url)
        for sensor in sensors:
            if len(sensor['values']):
                return re.search('(stable-[-0-9]+)', sensor['labels']['revision']).group(1)

    def get_current_used_bytes_count(self, cluster: str, disk_id: str) -> int:
        url = (f'{self._SOLOMON_URL}?project={self._SOLOMON_PROJECT}&who={self._SOLOMON_USER}'
               f'&cluster={cluster}&l.volume={disk_id}'
               f'&service=service_volume&l.sensor=UsedBytesCount&b=1m')

        r = requests.get(url)
        r.raise_for_status()

        sensors = r.json()['sensors']
        if len(sensors):
            if len(sensors[0]['values']):
                return sensors[0]['values'][-1]['value']
        return 0


class SolomonTestClient:

    def get_current_nbs_version(self, cluster: str, host: str) -> str:
        return "100.500"

    def get_current_used_bytes_count(self, cluster: str, disk_id: str) -> int:
        return 100500


def make_solomon_client(dry_run, logger):
    return SolomonTestClient() if dry_run else SolomonClient(logger)
