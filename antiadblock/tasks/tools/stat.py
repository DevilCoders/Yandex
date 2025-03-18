# -*- coding: utf-8 -*-
import os
from datetime import datetime, timedelta

import pandas as pd
import statface_client
import antiadblock.tasks.tools.common_configs as configs
from antiadblock.tasks.tools.calendar import Calendar

calendar = Calendar()


HISTORY_DAYS = 30
AGG_NAMES = ['_inner', '_outer', '_total']
VALUABLE_IDS = [
    'autoru',
    'yandex_images',
    'yandex_mail',
    'yandex_morda',
    'yandex_news',
    'yandex_player',
    'yandex_pogoda',
    'yandex_realty',
    'yandex_sport',
    'yandex_tv',
    'yandex_video',
    'turbo.yandex.ru',
    'zen.yandex.ru',
    'drive2',
]


class StatReports:

    def __init__(self, report_path='AntiAdblock/partners_money_united'):
        self.client = statface_client.StatfaceClient(host=statface_client.STATFACE_PRODUCTION, oauth_token=os.environ['STAT_TOKEN'])
        self.money_report = self.client.get_report(report_path)
        self._money_data = None

    def get_money_data_delay(self):
        """
        Проверяем, когда отчет обновлялся последний раз
        :return: minutes form last data timestamp
        """
        last_date_str = max(self.money_report.fetch_available_dates_range(scale='i')['available_dates'])
        last_date = datetime.strptime(last_date_str, configs.STAT_FIELDDATE_I_FMT)
        data_delay = (datetime.now() - last_date).seconds // 60
        return data_delay

    def get_money_data(self, since=None, until=None):
        holidays = calendar.get_nonwork_days(days=HISTORY_DAYS)
        now = datetime.now()
        since = since or (now - timedelta(days=HISTORY_DAYS)).strftime(configs.STAT_FIELDDATE_D_FMT)
        until = until or (now + timedelta(days=1)).strftime(configs.STAT_FIELDDATE_D_FMT)
        self._money_data = pd.DataFrame.from_records(
            self.money_report.download_data(
                scale='i',
                date_min=since,
                date_max=until,
                service_id__lvl=2,
            )
        )[['fielddate', 'service_id', 'aab_money', 'money', 'unblock']]
        self._money_data.loc[:, 'fielddate'] = pd.to_datetime(self._money_data['fielddate'])
        self._money_data.loc[:, 'service_id'] = self._money_data['service_id'].apply(lambda s: s.strip())
        self._money_data = self._money_data.query('service_id not in @AGG_NAMES')
        self._money_data = self._money_data[self._money_data.apply(lambda x: 'device' in x['service_id'], axis=1)]
        self._money_data['device'] = self._money_data.apply(lambda x: x['service_id'].split('\t')[2], axis=1)
        self._money_data['is_holiday'] = self._money_data.apply(lambda d: d['fielddate'].strftime('%Y-%m-%d') in holidays, axis=1)
        self._money_data.loc[:, 'service_id'] = self._money_data.apply(lambda x: x['service_id'].split('\t')[0], axis=1)
        self._money_data.set_index(['service_id', 'fielddate'], inplace=True)
        return self._money_data
