
from clan_tools.data_adapters.YTAdapter import YTAdapter
import pandas as pd
import requests
from datetime import datetime, timedelta
from urllib3.util.retry import Retry
from requests.adapters import HTTPAdapter
from joblib import Parallel, delayed
import itertools
import logging.config
from clan_tools.logging.logger import default_log_config
import click
from clan_tools.utils.timing import timing
import json

requests_proxy = {
    'http': 'http://zora.yandex.net:8166',
    'https': 'http://zora.yandex.net:8166',
}

logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


def isoformat(date):
    return date.to_pydatetime().isoformat('T', 'seconds')


class SpecializationVacancies:
    def __init__(self, per_page=100, specialization='1', days_before=30):
        self._per_page = per_page
        self._specialization = specialization
        self._date_from = datetime.now() - timedelta(days=days_before)
        self._date_to = datetime.now()
        self._time_range = pd.date_range(
            self._date_from, self._date_to, freq='1H')
        self._time_intervals = list(
            zip(self._time_range, self._time_range[1:]))

    def _get_vacancies_per_page(self, page, date_from, date_to):
        url = (
            f'http://api.hh.ru/vacancies'
            f'?specialization={self._specialization}'
            f'&per_page={self._per_page}'
            f'&date_from={isoformat(date_from)}'
            f'&date_to={isoformat(date_to)}'
            f'&page={page}'
        )
        result = None
        try:
            session = requests.Session()
            retry = Retry(connect=3, backoff_factor=0.5)
            adapter = HTTPAdapter(max_retries=retry)
            session.mount('http://', adapter)
            r = session.get(url, proxies=requests_proxy,
                            headers={
                                'X-Yandex-Sourcename': 'any',
                                'X-Yandex-Use-Https': '1',
                            })
            logger.debug(r)
            result = r.json()
        except:
            pass
        return result

    def _get_vacancies_per_time(self, time_interval):
        data = self._get_vacancies_per_page(
            page=0,
            date_from=time_interval[0],
            date_to=time_interval[1]
        )
        try:
            items = data['items']
            pages = data['pages']
        except KeyError:
            return []
        for i in range(1, pages):
            data = self._get_vacancies_per_page(
                page=i,
                date_from=time_interval[0],
                date_to=time_interval[1]
            )
            new_items = []
            try:
                new_items = data['items']
            except KeyError:
                logger.debug(data)
            items.extend(new_items)
        return items

    def get_vacancies(self):
        items_lists = Parallel(n_jobs=10, prefer="threads")(
            delayed(self._get_vacancies_per_time)(time_interval)
            for time_interval in self._time_intervals)
        items = list(itertools.chain(*items_lists))
        return items


@timing
@click.command()
@click.option('--result_table_path')
def main(result_table_path):
    specialization = SpecializationVacancies()
    vacancies = specialization.get_vacancies()
    vacancies_df = pd.DataFrame(vacancies,)

    YTAdapter().save_result(result_table_path,
                            schema=None, df=vacancies_df, default_type='any')

    with open('output.json', 'w') as f:
        json.dump({"table_path": result_table_path}, f)


if __name__ == "__main__":
    main()
