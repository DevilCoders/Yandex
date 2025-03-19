import os
import logging
import typing as tp

import requests  # type: ignore
import pandas as pd

logger = logging.getLogger(__name__)


class WikiAdapter:
    '''Can parse wiki pages which contains tables only'''

    def __init__(self, url: str = 'https://wiki-api.yandex-team.ru/_api/frontend/%s/.grid', token: tp.Optional[str] = None) -> None:
        '''
            To get token go to https://oauth.yandex-team.ru/authorize?response_type=token&client_id=ec9f4c29146640e6a1b01eccc5f84035
        '''
        self._url = url
        if token is None:
            token = os.environ['WIKI_TOKEN']
        self._token = token

    def get_data(self, table_path: str, to_pandas: bool = False) -> tp.Any:
        columns, rows = self._parse_wiki_table(table_path)
        if to_pandas:
            return pd.DataFrame(rows, columns=columns)
        else:
            return columns, rows

    def _parse_wiki_table(self, table_path: str = 'users/ktereshin/ProgramId-StreamId-Dictionary/') -> tp.Tuple[tp.List[str], tp.List[tp.List[tp.Any]]]:
        headers = {'Authorization': "OAuth %s" % self._token}
        url = self._url % table_path
        r = requests.get(url, headers=headers)
        j = r.json()
        columns = [f['title'] for f in j['data']['structure']['fields']]
        rows = [[c['raw'] for c in row] for row in j['data']['rows']]
        return columns, rows


def to_df(response: tp.Dict[str, tp.Any]) -> pd.DataFrame:
    cols = [x['title'] for x in response['data']['structure']['fields']]
    return pd.DataFrame([[x['raw'] for x in x_] for x_ in response['data']['rows']], columns=cols)
