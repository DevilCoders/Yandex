import os
from datetime import datetime

import statface_client


class Report:
    report_name = 'AntiAdblock/detect-checker-results'
    fielddate_fmt = '%Y-%m-%d %H:%M:00'

    def __init__(self):
        self.client = statface_client.StatfaceClient(
            host=statface_client.STATFACE_PRODUCTION,
            oauth_token=os.environ['STAT_TOKEN']  # how to get your token: https://wiki.yandex-team.ru/statbox/Statface/externalreports/#autentifikacija
        )
        self.report = self.client.get_report(self.report_name)
        self.check_time = datetime.utcnow()
        self.fielddate = self.check_time.strftime(self.fielddate_fmt)

    def upload_data(self, data):
        for check in data:
            check.update({'fielddate': self.fielddate})
        self.report.upload_data(scale='minutely', data=data)

    def get_checked_cases(self):
        last_check_data = self.report.download_data(scale='i', _period_distance=1)
        if not last_check_data:
            return []
        last_check_time_str = last_check_data[0]['fielddate']
        last_check_time = datetime.strptime(last_check_time_str, self.fielddate_fmt)
        if (self.check_time - last_check_time).seconds > 7200:
            return []
        self.fielddate = last_check_time_str  # write result to previously created timestamp
        checked_cases_set = set((check['browser'], check['browser_version'], check['adblock']) for check in last_check_data)
        checked_cases = [dict(browser=browser, browser_version=browser_version, adblock=adblock) for (browser, browser_version, adblock) in checked_cases_set]
        return checked_cases
