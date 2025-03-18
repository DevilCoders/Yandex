from xml.dom import minidom
from datetime import datetime, timedelta

import requests

from antiadblock.tasks.tools.logger import create_logger


class Calendar:

    query_templ = 'http://calendar.yandex-team.ru/export/holidays.xml?start_date={start_date}&end_date={end_date}&country_id=225&out_mode=all&who_am_i=robot-antiadb'
    holidays_fallback = [u'2019-11-04', u'2020-01-01', u'2020-01-02', u'2020-01-03', u'2020-01-04', u'2020-01-05',
                         u'2020-01-06', u'2020-01-07', u'2020-01-08', u'2020-02-23', u'2020-03-08', u'2020-05-01',
                         u'2020-05-09', u'2020-06-12', u'2020-07-01', u'2020-11-04']
    weekends_fallback = [u'2019-10-12', u'2019-10-13', u'2019-10-19', u'2019-10-20', u'2019-10-26', u'2019-10-27',
                         u'2019-11-02', u'2019-11-03', u'2019-11-09', u'2019-11-10', u'2019-11-16', u'2019-11-17',
                         u'2019-11-23', u'2019-11-24', u'2019-11-30', u'2019-12-01', u'2019-12-07', u'2019-12-08',
                         u'2019-12-14', u'2019-12-15', u'2019-12-21', u'2019-12-22', u'2019-12-28', u'2019-12-29',
                         u'2020-01-11', u'2020-01-12', u'2020-01-18', u'2020-01-19', u'2020-01-25', u'2020-01-26',
                         u'2020-02-01', u'2020-02-02', u'2020-02-08', u'2020-02-09', u'2020-02-15', u'2020-02-16',
                         u'2020-02-22', u'2020-02-24', u'2020-02-29', u'2020-03-01', u'2020-03-07', u'2020-03-09',
                         u'2020-03-14', u'2020-03-15', u'2020-03-21', u'2020-03-22', u'2020-03-28', u'2020-03-29',
                         u'2020-04-04', u'2020-04-05', u'2020-04-11', u'2020-04-12', u'2020-04-18', u'2020-04-19',
                         u'2020-04-25', u'2020-04-26', u'2020-05-02', u'2020-05-03', u'2020-05-04', u'2020-05-05',
                         u'2020-05-10', u'2020-05-11', u'2020-05-16', u'2020-05-17', u'2020-05-23', u'2020-05-24',
                         u'2020-05-30', u'2020-05-31', u'2020-06-06', u'2020-06-07', u'2020-06-13', u'2020-06-14',
                         u'2020-06-20', u'2020-06-21', u'2020-06-27', u'2020-06-28', u'2020-07-04', u'2020-07-05',
                         u'2020-07-11', u'2020-07-12', u'2020-07-18', u'2020-07-19', u'2020-07-25', u'2020-07-26',
                         u'2020-08-01', u'2020-08-02', u'2020-08-08', u'2020-08-09', u'2020-08-15', u'2020-08-16',
                         u'2020-08-22', u'2020-08-23', u'2020-08-29', u'2020-08-30', u'2020-09-05', u'2020-09-06',
                         u'2020-09-12', u'2020-09-13', u'2020-09-19', u'2020-09-20', u'2020-09-26', u'2020-09-27',
                         u'2020-10-03', u'2020-10-04', u'2020-10-10', u'2020-10-11', u'2020-10-17', u'2020-10-18',
                         u'2020-10-24', u'2020-10-25', u'2020-10-31', u'2020-11-01', u'2020-11-07', u'2020-11-08',
                         u'2020-11-14', u'2020-11-15', u'2020-11-21', u'2020-11-22', u'2020-11-28', u'2020-11-29',
                         u'2020-12-05', u'2020-12-06', u'2020-12-12', u'2020-12-13', u'2020-12-19', u'2020-12-20',
                         u'2020-12-26', u'2020-12-27']
    data = None

    def __init__(self, logger=None):
        self.logger = logger or create_logger('calendar')

    def __get_calendar_data(self, **period):
        end_date = datetime.now()
        start_date = end_date - timedelta(**period)

        query = self.query_templ.format(start_date=start_date.strftime("%Y-%m-%d"),
                                        end_date=end_date.strftime("%Y-%m-%d"))
        response = requests.get(query, timeout=3)
        self.data = minidom.parseString(response.text.encode('utf-8'))

    def get_nonwork_days(self, force_fallback=False, holidays=True, weekends=True, **period):
        result = []
        if force_fallback:
            if holidays:
                result += self.holidays_fallback
            if weekends:
                result += self.weekends_fallback
            return result
        if self.data is None:
            self.__get_calendar_data(**period)
        days = self.data.getElementsByTagName("day")
        for day in days:
            if day.getAttribute('is-holiday') == '1' and ((holidays and day.getAttribute('day-type') == 'holiday') or (weekends and day.getAttribute('day-type') == 'weekend')):
                result.append(day.getAttribute('date'))
        return result
