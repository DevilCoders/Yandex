import os
import json
import logging
from typing import Dict, Optional, Any

import requests  # type: ignore
from requests.models import Response  # type: ignore

logger = logging.getLogger(__name__)


class ChartsAdapter:
    def __init__(self, url: str = "https://api.charts.yandex.net/v1/charts/", token: Optional[str] = None):
        '''
        Realization of https://datalens.yandex-team.ru/docs/api/charts/.
        Get token from  https://datalens.yandex-team.ru/docs/api/auth or better use robot's token.
        '''
        self._url = url
        if token is None:
            self._token = os.environ['DATALENS_TOKEN']
        else:
            self._token = token

    def update_chart(self, chart_id: str, data: Dict[str, Any]) -> Response:
        """
        Update Charts with chart_id. Based on https://datalens.yandex-team.ru/docs/api/charts/
        """
        try:
            response_ = requests.post(
                url=f'{self._url}{chart_id}',
                headers={
                    "Authorization": "OAuth %s" % (self._token),
                    "Content-Type": "application/json; charset=utf-8",
                },
                data=json.dumps(data), verify=False)
            logger.debug('Response HTTP Status Code: {status_code}'.format(
                status_code=response_.status_code))
            logger.debug('Response HTTP Response Body: {content}'.format(
                content=response_.content))
            logger.debug(response_)
            return response_
        except requests.exceptions.RequestException as e:
            logger.error(e)

    def fetch_chart_data(self, chart_id: str, page: Optional[int] = None) -> Dict[str, Any]:
        response = None
        payload: Dict[str, Any] = {'id': chart_id}
        if page is not None:
            payload['params'] = {'_page': page}
        response = requests.post(
            url="https://charts.yandex-team.ru/api/run",
            headers={
                "Authorization": f"OAuth {self._token}",
                "Content-Type": "application/json; charset=utf-8",
            },
            data=json.dumps(payload)
        )
        status_code = response.status_code
        logger.debug(
            f'Response for {chart_id}.'
            f' {"" if page is None else "Page " + str(page)}.'
            f' HTTP Status Code: {status_code}'
        )
        if status_code != 200:
            logger.error(f'Response: {response.text}')
        return response.json()
