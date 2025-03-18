# coding: utf-8
import os
from collections import defaultdict

import statface_client


REPORT_URL_TEMPLATE = "https://stat.yandex-team.ru/AntiAdblock/detect-checker-results?scale=i&service_id={}&browser=_in_table_&browser_version=_in_table_" \
                      "&adblock=_in_table_&_period_distance=1&_incl_fields=console_detect_result"


def get_detect_checker_info(service_id, date_start):
    stat_client = statface_client.StatfaceClient(host=statface_client.STATFACE_PRODUCTION,
                                                 oauth_token=os.getenv('STAT_TOKEN'))
    report = stat_client.get_report('AntiAdblock/detect-checker-results')
    data = report.download_data(scale='i', service_id=service_id, _field=['browser', 'adblock', 'console_detect_result', 'fielddate'])
    table_data = defaultdict(list)
    unique_browsers = set()
    for row in data:
        table_data[row['adblock']].append((row['browser'], row['console_detect_result']))
        unique_browsers.add(row['browser'])
    table_content = u'|| Блокировщик/Браузер|' + '|'.join(sorted(list(unique_browsers))) + '||\n'
    for adblock, browsers in table_data.items():
        table_row = '|| {}|'.format(adblock) + '|'.join(map(lambda b: b[1], sorted(browsers))) + '||\n'
        table_content += table_row
    table_content = '#|\n' + table_content + '|#\n'
    link = '\n((https://stat.yandex-team.ru/AntiAdblock/detect-checker-results?scale=i&service_id=autoru&browser=_in_table_&' \
           'browser_version=_in_table_&adblock=_in_table_&_period_distance=1&_incl_fields=console_detect_result Актуальный отчет))'
    return table_content.encode('utf-8') + link
