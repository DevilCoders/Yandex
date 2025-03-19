#!/usr/bin/env python
# -*- encoding: utf-8 -*-


from __future__ import unicode_literals, print_function

import argparse
import logging
import re
import os
import sys
import time
import xml.etree.cElementTree as ET
from collections import Counter
from contextlib import contextmanager
from datetime import timedelta

import arrow
import requests
import yaml

import ticket_parser2.api.v1 as tp2

import requests.packages.urllib3

from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

requests.packages.urllib3.disable_warnings()

log = logging.getLogger()

DEFAULT_TIMEOUT = 5 # seconds

class TimeoutHTTPAdapter(HTTPAdapter):
    def __init__(self, *args, **kwargs):
        self.timeout = DEFAULT_TIMEOUT
        if "timeout" in kwargs:
            self.timeout = kwargs["timeout"]
            del kwargs["timeout"]
        super(TimeoutHTTPAdapter,self).__init__(*args, **kwargs)

    def send(self, request, **kwargs):
        timeout = kwargs.get("timeout")
        if timeout is None:
            kwargs["timeout"] = self.timeout
        return super(TimeoutHTTPAdapter,self).send(request, **kwargs)

class DumbService(object):
    base_url = None
    tvm2_token = ''

    def __init__(self, token=None, tvm2=False):
        self.headers = {'Authorization': 'OAuth {}'.format(token)} if token else {}
        if tvm2:
            tvm2_client = tp2.TvmClient(tp2.TvmApiClientSettings(
                self_client_id=2000273,
                enable_service_ticket_checking=True,
                enable_user_ticket_checking=tp2.BlackboxEnv.Test,
                self_secret=self.tvm2_token,
                dsts={"calendar": 2011072},
            ))
            self.headers['X-Ya-Service-Ticket'] = tvm2_client.get_service_ticket_for("calendar")
        self.session = requests.Session()
        self.retries = Retry(
            total=3,
            status_forcelist=[429, 500, 502, 503, 504],
            method_whitelist=["HEAD", "GET", "POST", "PATCH", "DELETE", "OPTIONS"],
            backoff_factor=1
        )
        self.adapter = TimeoutHTTPAdapter(max_retries=self.retries)
        self.session.mount("https://", self.adapter)
        self.session.mount("http://", self.adapter)

    def json_request(self, method, path, **kwargs):
        name = self.__class__.__name__
        r = self._request(method, path, **kwargs)
        try:
            res = r.json()
        except ValueError:
            raise
        if 'error' in res and res['error']:
            raise RuntimeError('{} request failed: {}'.format(name, res['error']))
        if 'status' in res and isinstance(res['status'], basestring) and res['status'] not in ['OK', 'Created']:
            raise RuntimeError('{} request failed: {}'.format(name, res['status']))
        if 'result' in res:
            res = res['result']
        if 'results' in res:
            res = res['results']
        return res

    def _request(self, method, path, **kwargs):
        name = self.__class__.__name__
        _kwargs = {
            'headers': self.headers,
            'verify': False,  # FIXME: set ca path and fix SNI
        }
        _kwargs.update(kwargs)
        r = self.session.request(method, '{}/{}'.format(self.base_url.rstrip('/'),
                                                        path.lstrip('/')),
                                 **_kwargs)
        r.raise_for_status()
        return r

# TODO: public calendar access without layer_token
class Calendar(DumbService):
    base_url = 'https://calendar-api.tools.yandex.net/api'
    tvm2_token = ''

    def __init__(self, layer_token):
        self.params = {
            'layer-private-token': layer_token,
        }
        self.time_format = 'YYYY-MM-DDTHH:mm:ssZ'
        super(Calendar, self).__init__(tvm2=True)

    def _fmt_time(self, t):
        return arrow.get(t).format(self.time_format)

    def _request(self, method, path, **kwargs):
        params = {}
        params.update(self.params)
        params.update(kwargs.get('params', {}))
        kwargs['params'] = params
        kwargs['verify'] = False
        return super(Calendar, self)._request(method, path, **kwargs)

    def xml_request(self, method, path, **kwargs):
        name = self.__class__.__name__
        res = super(Calendar, self)._request(method, path, **kwargs)
        element = ET.fromstring(res.text)
        if element.tag == 'error':
            raise RuntimeError('{} request failed: {}'.format(name, element.attrib))
        return element.attrib

    def get_holidays(self, start, end, tz):
        params = {
            'startDate': arrow.get(start).format('YYYY-MM-DD'),
            'endDate': arrow.get(end).format('YYYY-MM-DD'),
            'countryId': 225,  # FIXME: magic number for Russia
            'outMode': 'holidays',
        }
        holidays = self.json_request('GET', 'get-holidays.json', params=params)
        return [arrow.get(i['date']).replace(tzinfo=tz).floor('day') for i in holidays['days']]

    def get_events(self, start, end):
        params = {
            'from': self._fmt_time(start),
            'to': self._fmt_time(end),
        }
        return self.json_request('GET', 'get-events-on-layer.json', params=params)['events']


class Conditions(object):
    def __init__(self, oauth_token, conditions_config, start_date, end_date, history_events=[]):
        self._oauth_token = oauth_token
        self.start_date = start_date
        self.end_date = end_date
        self.config = conditions_config
        self._calendar_events_cache = {}
        self._absence_cache = {}
        self._holidays_cache = {}
        self._history_events = history_events  # mutable cache of calendar generated at current run

    def _get_events(self, layer_token):
        if layer_token not in self._calendar_events_cache:
            c = Calendar(layer_token)
            self._calendar_events_cache[layer_token] = c.get_events(self.start_date,
                                                                    self.end_date)
        return self._calendar_events_cache[layer_token]

    def _get_absences(self, user):
        if user not in self._absence_cache:
            gaps = Gap(self._oauth_token).gaps_find([user],
                                                    self.start_date,
                                                    self.end_date)
            self._absence_cache.update(gaps)
        return self._absence_cache[user]

    def _get_holidays(self, tz):
        if tz not in self._holidays_cache:
            calendar = Calendar('who care')
            self._holidays_cache[tz] = calendar.get_holidays(self.start_date,
                                                             self.end_date,
                                                             tz=tz)
        return self._holidays_cache[tz]

    def suitable(self, user, start, end):
        return sum([
            self._suitable(self.config.get(user, {}), user, start, end),
            self._suitable(self.config.get('ALL', {}), user, start, end),
        ])

    def _suitable(self, config_part, user, start, end):
        weight = 0
        for cond, config in config_part.iteritems():
            if not isinstance(config, dict):
                config = {'args': config}
            if not self._call(user, start, end, cond, config.get('args') or []):
                weight -= config.get('weight', 99)  # FIXME: magic number
        return weight

    def is_duty_day(self, start, end):
        if self.config.get('DUTY_DAYS'):
            return self._suitable(self.config.get('DUTY_DAYS'), None, start, end) >= 0
        return True

    def _call(self, user, start, end, method, args):
        args = args or []
        res = getattr(self, method)(user, start, end, *args)
        return res

    def weekday_in(self, user, start, end, *weekdays):
        return start.isoweekday() in weekdays

    def weekday_not_in(self, user, start, end, *weekdays):
        return not self.weekday_in(user, start, end, *weekdays)

    def day_in(self, user, start, end, *days):
        return start.day in days

    def day_not_in(self, user, start, end, *days):
        return not self.day_in(user, start, end, *days)

    def on_duty_in_calendar(self, user, start, end, layer_token):
        events = self._get_events(layer_token)
        for event in events:
            e_start, e_end = arrow.get(event['start']), arrow.get(event['end'])
            if range_intersect([start, end], [e_start, e_end]) > timedelta() and user in event['name']:
                return True
        return False

    def not_duty_in_calendar(self, user, start, end, layer_token):
        events = self._get_events(layer_token)
        for event in events:
            e_start, e_end = arrow.get(event['start']), arrow.get(event['end'])
            if range_intersect([start, end], [e_start, e_end]) > timedelta() and user in event['name']:
                return False
        return True

    def not_absent(self, user, start, end):
        absences = self._get_absences(user)
        return not any(range_intersect([start, end], a) > timedelta() for a in absences)

    def not_holidays(self, user, start, end, tz):
        holidays = self._get_holidays(tz)
        return start.floor('day') not in holidays

    def weekday_as_prev(self, user, start, end, *weekdays):
        if self.weekday_in(user, start, end, *weekdays):
            duty_name, end, start = who_duty(self._history_events,
                                             start.replace(days=-1),
                                             end.replace(days=-1))
            if duty_name == user:
                return True
        return False

    def not_in_row(self, user, start, end):
        duty_name, end, start = who_duty(self._history_events,
                                         start.replace(days=-1),
                                         end.replace(days=-1))
        if duty_name == user:
            return False
        return True

def range_intersect(a, b):
    return min(b[1], a[1]) - max(b[0], a[0])

def load_config(path):
    try:
        return yaml.load(open(path))
    except Exception as e:
        raise argparse.ArgumentTypeError(e)

def check_calendar(config, range, tz, start_hour):
    Calendar.tvm2_token = config['tvm2_token']
    calendar = Calendar(config['calendar_layer_token'])
    conditions = Conditions(config['oauth_token'],
                            config['conditions'],
                            arrow.now(tz=tz).replace(hour=start_hour),
                            arrow.now(tz=tz).replace(hour=start_hour, days=range),
                            None
    )
    for start in arrow.Arrow.range('day', arrow.now(tz=tz).replace(hour=start_hour), limit=range):
        end = start.replace(days=1)
        if conditions.is_duty_day(start, end):
            if not calendar.get_events(start, end):
                return config['team']
    return None

if __name__ == '__main__':
    config_dir = "/etc/yandex/duty-bot"
    range = 14
    tz = 'Europe/Moscow'
    start_hour = 11
    errors = []
    params = {}
    try:
        for config_file in os.listdir(config_dir):
            if not "strm" in config_file:
                config = load_config(os.path.join(config_dir,config_file))
                res = check_calendar(config, range, tz, start_hour)
                if res:
                    errors.append(res)
        if errors:
            print("2;Gaps in calendar(s) for {} days: {}".format(range, ",".join(errors)))
        else:
            print("0;OK")
    except Exception as err:
        print("2;{}".format(err))