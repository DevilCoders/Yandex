# -*- coding: utf-8 -*-

import datetime
import json
import requests

from cloud.iam.planning_tool.library.clients import RestClient


class HolidayCalendar:
    def __init__(self, config):

        response = RestClient(config).session.get(f'{config["url"]}/get-holidays',
                                                  params={'from': '2015-01-01',
                                                          'to': '2050-01-01',
                                                          'for': 'rus',
                                                          'outMode': 'holidays'})
        if response.status_code != requests.codes.ok:
            response.raise_for_status()

        self._holidays = set(
            map(lambda x: datetime.date.fromisoformat(x['date']), json.loads(response.text)['holidays']))

    def is_holiday(self, date):
        return date in self._holidays

    def is_work_day(self, date):
        return not self.is_holiday(date)

    def previous_work_day(self, date):
        result = date - datetime.timedelta(days=1)
        while self.is_holiday(result):
            result = result - datetime.timedelta(days=1)
        return result

    def next_work_day(self, date):
        result = date + datetime.timedelta(days=1)
        while self.is_holiday(result):
            result = result + datetime.timedelta(days=1)
        return result
