from functools import lru_cache
from typing import Dict

from cloud.dwh.clients.base.http import AsyncApiClient

ENDPOINT = 'https://solomon.yandex-team.ru/api/v2'


class SolomonApiClient(AsyncApiClient):
    """
    https://solomon.yandex-team.ru/swagger-ui/index.html
    """

    @staticmethod
    def dict2selector(data: Dict[str, str]) -> str:
        """
        Converts dictionary {'a': 1, 'b': 2} to solomon selector {a="1",b="2"}
        """
        return f'''{{{",".join(f'{key}="{value}"' for key, value in data.items())}}}'''

    async def get_labels(self, project: str, params: dict) -> dict:
        """
        https://solomon.yandex-team.ru/swagger-ui/index.html#/sensors/findAllLabelValuesInOldFormatUsingGET
        """
        return await self._http_client.get(f'/projects/{project}/sensors/labels', params=params)

    async def sensors(self, project: str, params: dict) -> dict:
        """
        https://solomon.yandex-team.ru/swagger-ui/index.html#/sensors/findMetricsUsingGET
        """
        return await self._http_client.get(f'/projects/{project}/sensors', params=params)

    async def data(self, project: str, body: dict) -> dict:
        """
        https://solomon.yandex-team.ru/swagger-ui/index.html#/data/readDataFromJsonUsingPOST
        """
        return await self._http_client.post(f'/projects/{project}/sensors/data', body=body)


@lru_cache
def get_solomon_client(endpoint: str = ENDPOINT, token: str = None) -> 'SolomonApiClient':
    return SolomonApiClient(endpoint=endpoint, token=token)
