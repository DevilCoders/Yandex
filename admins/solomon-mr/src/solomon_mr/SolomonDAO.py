from typing import List, Dict

import attr
import logging
import requests
import yaml

from src.solomon_mr.DataPoint import DataPoint, DataPointKV, DataPointLabels


class SolomonDAO:
    _API = 'https://solomon.yandex.net/api/v2/'

    def __init__(self, token: str, output: DataPointLabels):
        self._log = logging.getLogger(self.__class__.__name__)
        self._token = token
        self._output = output

    def _post(self, url: str, payload, params: Dict[str, str] = None):
        """ Generic API post """
        resp = requests.post(self._API + url,
                             params=params,
                             json=payload,
                             headers={'Authorization': 'OAuth ' + self._token})
        resp.raise_for_status()
        return resp.json()

    def get_data(self, timestamp: int, params: DataPointLabels, ttl_secs: int) -> List[DataPoint]:
        """ Get data from solomon """
        try:
            program = ', '.join(f"{k}='{v}'" for (k, v) in attr.asdict(params).items())
            payload = {
                'program': '{' + program + '}',
                'from': (timestamp - ttl_secs) * 1000,  # ms
                'to': timestamp * 1000,  # ms
            }
            project = self._output.project
            json = self._post(f'projects/{project}/sensors/data', payload)
            timeseries = json['vector'][0]['timeseries']
            data = zip(timeseries['timestamps'], timeseries['values'])
            return [DataPoint(timestamp=item[0] // 1000,  # ms
                              labels=timeseries['labels'],
                              value=item[1]) for item in data]
        except (KeyError, IndexError):
            return []

    def submit_data(self, timestamp: int, checks: List[DataPointKV]) -> None:
        """ Submit check statuses back to Solomon """
        sensors = []
        for point in checks:
            sensors.append({
                'labels': {'sensor': point.sensor},
                'value': point.value,
                'ts': timestamp,  # secs(!)
            })
        result = {'sensors': sensors}

        resp = self._post('push', payload=result, params=attr.asdict(self._output))
        for line in yaml.safe_dump(resp).strip().split('\n'):
            self._log.debug(line)
