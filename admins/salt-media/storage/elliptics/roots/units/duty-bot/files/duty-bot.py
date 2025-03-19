#!/usr/bin/env python
# -*- encoding: utf-8 -*-


from __future__ import unicode_literals, print_function

import argparse
import logging
import re
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

from requests.packages.urllib3.exceptions import InsecureRequestWarning

from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

requests.packages.urllib3.disable_warnings()

log = logging.getLogger()

nobody_dict = {
    'work_phone': 'никакой',
    'login': 'никто',
    'telegram': 'никакой'
}


def admin_by_phone(admins, phone):
    if not phone:
        return None
    if phone.startswith('55'):
        phone = phone[2:]
    elif phone.startswith('*55'):
        phone = phone[3:]
    admin = next(admin for admin in admins if admin['work_phone'] == int(phone))
    return admin

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
            log.error('Invalid json: {}'.format(r.text))
            raise
        if 'error' in res and res['error']:
            raise RuntimeError('{} request failed ({}): {}'.format(name, r.status_code, res['error']))
        if 'status' in res and isinstance(res['status'], basestring) and res['status'] not in ['OK', 'Created']:
            raise RuntimeError('{} request failed ({}): {}'.format(name, r.status_code, res['status']))
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
        log.debug('{}: request {} {} {}'.format(name, method, path, _kwargs))
        r = self.session.request(method, '{}/{}'.format(self.base_url.rstrip('/'),
                                                        path.lstrip('/')),
                                 **_kwargs)
        log.debug('{} response: {}'.format(name, repr(r.text)))
        r.raise_for_status()
        return r


class Staff(DumbService):
    base_url = 'https://staff-api.yandex-team.ru/v3'

    def list_users_by_department(self, department):
        params = {
            '_fields': 'person.login,person.official.is_dismissed,person.official.is_robot',
            'group.url': department
        }
        res = self.json_request('GET', 'groupmembership', params=params, verify=False)
        return [
            v['person']['login']
            for v in res
            if not any(v['person']['official'].values())  # wtf?
        ]

    def get_users_data(self, users):
        params = {
            '_fields': 'login,work_phone,accounts',
            'login': ','.join(users),
        }
        staff_data = self.json_request('GET', 'persons', params=params)
        result = []
        for user in staff_data:
            accounts = user.pop('accounts')
            for acc in accounts:
                if not acc['private']:
                    user[acc['type']] = acc['value']
            result.append(user)
        return result


class Gap(DumbService):
    base_url = 'https://staff.yandex-team.ru/gap-api/api'

    @staticmethod
    def _fmt_time(t):
        return arrow.get(t).format('YYYY-MM-DDTHH:mm:ssZ')

    def gaps_find(self, logins, start, end):
        params = {
            'person_login': logins,
            'date_from': self._fmt_time(start),
            'date_to': self._fmt_time(end),
        }
        gaps = self.json_request('GET', 'gaps_find', params=params)['gaps']
        res = {k: list() for k in logins}
        for g in gaps:
            if not g['work_in_absence']:
                res[g['person_login']].append([
                    arrow.get(g['date_from']),
                    arrow.get(g['date_to']),
                ])
        return dict(res)


class Telegraph(DumbService):
    base_url = 'https://telegraph.yandex-team.ru/api/v3'

    def get_forward(self, phone):
        res = self.json_request('GET',
                                'cucm/translation_pattern/pattern/{}'.format(phone))
        return res['calledPartyTransformationMask'] or None

    def set_forward(self, phone, to_phone):
        log.debug('Set forward {} -> {}'.format(phone, to_phone))
        if to_phone is None:
            to_phone = ''
        data = {'calledPartyTransformationMask': to_phone}
        return self.json_request('PUT',
                                 'cucm/translation_pattern/pattern/{}'.format(phone),
                                 json=data)


# TODO: public calendar access without layer_token
class Calendar(DumbService):
    base_url = 'https://calendar-api.tools.yandex.net/api'
    # base_url = 'https://calendar-api.calcorp-test-back.yandex-team.ru:81'
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
        # https://calendar.yandex-team.ru/export/holidays.xml?start_date=2018-02-01\
        #                                                    &end_date=2018-03-01\
        #                                                    &country_id=225\
        #                                                    &out_mode=holidays\
        #                                                    &who_am_i=duty-bot
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

    def get_now_events(self):
        """
            "events": [
            {
                "description": "script generated",
                "end": "2018-05-23T11:00:00.000+03:00",
                "id": "32849169",
                "location": "",
                "name": "agodin@ duty",
                "participants": [
                    {
                        "decision": "yes",
                        "email": "agodin@yandex-team.ru",
                        "is-organizer": "false",
                        "is-resource": "false",
                        "login": "agodin",
                        "name": "\u0410\u043d\u0434\u0440\u0435\u0439 \u0413\u043e\u0434\u0438\u043d"
                    }
                ],
                "start": "2018-05-22T11:00:00.000+03:00"
            }
        """
        res = self.json_request('GET', 'get-now-events-on-layer.json')
        return res['events']

    def create_event(self, name, start, end, description=None):
        log.debug('Create calendar event: {} {} -- {}'.format(name, start, end))
        params = {
            'name': name,
            'start': arrow.get(start).format(self.time_format),
            'end': arrow.get(end).format(self.time_format),
            'description': description,
        }
        return self.json_request('POST', 'create-event-on-layer.json', params=params)['event-id']

    def share_event(self, event_id, email):
        params = {
            'id': event_id,
            'email': email if '@' in email else '{}@yandex-team.ru'.format(email)
        }
        return self.xml_request('POST', 'share-event.xml', params=params)

    def delete_event(self, event_id, email='robot-storage-duty@yandex-team.ru'):
        params = {
            'email': email if '@' in email else '{}@yandex-team.ru'.format(email),  # strange api
            'id': event_id,
        }
        return self.xml_request('POST', 'delete-event.xml', params=params)

    def clean(self, start, end):
        for event in self.get_events(start, end):
            # FIXME:
            m = re.search(r'(?P<login>[-\w]+)@', event['name'])
            if m:
                self.delete_event(event['id'])

    def share(self, start, end):
        for event in self.get_events(start, end):
            # FIXME:
            m = re.search(r'(?P<login>[-\w]+)@', event['name'])
            if m:
                self.share_event(event['id'], m.group(b'login'))

class Golem(DumbService):
    base_url = 'https://golem.yandex-team.ru/api'

    def get_resps(self, obj):
        return self.json_request('GET', 'get_object_resps.sbml', params={'object': obj})['resps']

    def set_resps(self, obj, resps):
        data = {
            'resps': resps,
            'object': obj,
        }
        return self.json_request('POST', 'set_object_resps.sbml', json=data)

    def get_my_hosts(self, resp):
        r = self._request('GET', 'host_query.sbml', params={'host_resp': resp})
        return r.text.strip().split()

    def move_resp_to_position(self, resp, pos='last'):
        """Put `resp` at position `pos` in resps list"""
        obj_list = self.get_my_hosts(resp)
        for obj in obj_list:
            old_resp_list = [r['name'] for r in self.get_resps(obj)]
            resp_list = [r for r in old_resp_list if r != resp]
            pos = {'last': len(resp_list), 'first': 0}.get(pos, pos)
            resp_list.insert(pos, resp)
            if old_resp_list != resp_list:
                log.debug('Change resps from {} to {}'.format(old_resp_list, resp_list))
                self.set_resps(obj, resp_list)


class Juggler(DumbService):
    base_url = 'https://juggler-api.search.yandex.net/api'
    params = {'do': '1', 'format': 'json'}

    def get_notify_rules(self, filters):
        # http://juggler-api.search.yandex.net/api/notify_rules/get_notify_rules
        path = 'notify_rules/get_notify_rules'
        return self.json_request('POST', path, json={'filters': filters}, params=self.params)

    def add_or_update_notify_rule(self, rule):
        # http://juggler-api.search.yandex.net/api/notify_rules/add_or_update_notify_rule
        path = 'notify_rules/add_or_update_notify_rule'
        return self.json_request('POST', path, json=rule, params=self.params)

    def get_notify_rule_by_id(self, rule_id):
        return self.get_notify_rules(filters=[{'rule_id': rule_id}])['rules'][0]

    def set_rule_logins(self, rule_id, logins):
        # rotates logins list of rule with given rule_id until given login became first
        rule = self.get_notify_rule_by_id(rule_id)
        if rule['template_name'] == 'on_status_change':
            del rule['template_kwargs']['login']
            rule['template_kwargs']['login'] = logins
        else:
            del rule['template_kwargs']['logins']
            rule['template_kwargs']['logins'] = logins
        del rule['creation_time']
        del rule['hints']
        return self.add_or_update_notify_rule(rule)

    def get_current_logins(self, rule_id):
        res = self.get_notify_rule_by_id(rule_id)['template_kwargs']
        if res.get('logins'):
            return res['logins']
        else:
            return res['login']


class ABC(DumbService):
    base_url = 'https://abc-back.yandex-team.ru/api'

    def __init__(self,token=None,**kwargs):
        self.params = {}
        for key,value in kwargs.items():
            self.params[key] = value
        super(ABC, self).__init__(token=token)

    def get_current_duty(self, service_id, service_slug=None, schedule_id=None, schedule_slug=None):
        path = 'v4/duty/on_duty/'
        params = {
            'service': service_id,
            'service__slug': service_slug,
            'schedule': schedule_id,
            'schedule__slug': schedule_slug,
        }
        res = self.json_request('GET', path, params=params)
        if res:
            return res[0]['person']['login']

    def get_responsibles(self, service_id, service_slug=None):
        path = 'v4/services/responsibles/'
        params = {
            'service': service_id,
            'service__slug': service_slug
        }
        return self.json_request('GET', path, params=params)
        

    def get_shifts(self, date_from, date_to):
        path = 'v4/duty/shifts/'
        params = {
            'date_from': date_from.format("YYYY-MM-DD"),
            'date_to': date_to.format("YYYY-MM-DD"),
            'service': self.params['service_id'],
            'schedule': self.params['schedule_id'],
        }
        res = []
        for shift in self.json_request('GET', path, params=params, timeout=5):
            if arrow.get(shift['start_datetime']).floor('day').datetime >= date_from.floor('day').datetime and arrow.get(shift['end_datetime']).floor('day').datetime <= date_to.floor('day').datetime:
                res.append(shift)
        return res

    def delete_shift(self, shift_id):
        path = 'v4/duty/shifts/{}/'.format(shift_id)
        return self._request('DELETE', path, timeout=5)

    def cleanup_shifts(self, date_start, date_end):
        for shift in self.get_shifts(date_start, date_end):
            self.delete_shift(shift['id'])
            log.info("Shift #{}({},{}) deleted from ABC duty calendar".format(shift['id'],shift['person'] if shift['person'] is None else shift['person']['login'],shift['start']))

    def append_shift(self, admin, date_start, date_end):
        path = 'v4/duty/schedules/{}/shifts/append/'.format(self.params['schedule_id'])
        data = [{
            'person': admin,
            'schedule': self.params['schedule_id'],
            'start_datetime': date_start.format("YYYY-MM-DDTHH:mm:ss"),
            'end_datetime': date_end.format("YYYY-MM-DDTHH:mm:ss")
        }]
        return self._request('POST', path, json=data, timeout=5)

    def sync_shift(self, admin, date_start, date_end):
        shifts = self.get_shifts(date_start, date_end)
        if shifts:
            if shifts[0]['person']['login'] != admin:
                self.delete_shift(shifts[0]['id'])
                self.append_shift(admin, date_start, date_end)
                log.info("ABC sync: deleted shift {} ({}). Added shift {}".format(shifts[0]['id'], shifts[0]['person']['login'], admin))
        else:
            self.append_shift(admin, date_start, date_end)
            log.info("ABC sync: Added shift {}".format(admin))

    def update_calendar(self, **kwargs):
        path = 'v4/duty/schedules/{}/'.format(self.params['schedule_id'])
        data = {}
        for key,value in kwargs.items():
            data[key] = value
        return self._request('PATCH', path, json=data, timeout=5)

class Startrek(DumbService):
    base_url = 'https://st-api.yandex-team.ru/v2'

    def __init__(self, config):
        self.queue = config['queue']
        self.summary_template = config['summary']
        super(Startrek, self).__init__(config['oauth_token'])

    def create_ticket(self, next_admin=None):
        path = 'issues/'
        context = {
            'next_admin': next_admin or nobody_dict,
            'date': arrow.now().format('DD.MM.YYYY')
        }
        data = {
            'queue': self.queue,
            'summary': self.summary_template.format(**context),
            'assignee': next_admin['login']
        }
        res = self.json_request('POST', path, json=data)
        if res:
            return res['key']

    def update_issue(self, task_id, **kwargs):
        path = 'issues/{}/'.format(task_id)
        data = {}
        for key,value in kwargs.items():
            data[key] = value
        return self._request('PATCH', path, json=data, timeout=5)

    def search_issue(self, **kwargs):
        path = 'issues/'
        params = {}
        for key,value in kwargs.items():
            params[key] = value
        page = 1
        total_pages = 1
        while page <= total_pages:
            params['page'] = page
            r = self._request('GET', path, params=params, timeout=60)
            for issue in r.json():
                yield issue
            total_pages = int(r.headers.get("X-Total-Pages",1))
            page += 1

    def workflow_issue(self, task_id, action, **kwargs):
        path = 'issues/{}/transitions/{}/_execute'.format(task_id,action)
        data = {}
        for key,value in kwargs.items():
            data[key] = value
        return self.json_request('POST', path, json=data, timeout=60)

    def comment_issue(self, task_id, **kwargs):
        path = 'issues/{}/comments'.format(task_id)
        data = {}
        for key,value in kwargs.items():
            data[key] = value
        return self.json_request('POST', path, json=data, timeout=60)


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
            else:
                weight += config.get('weight', 0)
        return weight

    def is_duty_day(self, start, end):
        if self.config.get('DUTY_DAYS'):
            return self._suitable(self.config.get('DUTY_DAYS'), None, start, end) >= 0
        return True

    def _call(self, user, start, end, method, args):
        args = args or []
        res = getattr(self, method)(user, start, end, *args)
        log.debug('Check {}({}) = {} for {} in range {} - {}'.format(method,
                                                                     args,
                                                                     res,
                                                                     user,
                                                                     start,
                                                                     end))
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
            # log.debug('Weekday_as_prev: duty={} user={} start={} end={}'.format(duty_name,
            #                                                                     user,
            #                                                                     start,
            #                                                                     end))
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


def load_config(path):
    try:
        return yaml.load(open(path))
    except Exception as e:
        raise argparse.ArgumentTypeError(e)


def who_duty_action(args):
    config = args.config
    # staff = Staff(config['oauth_token'])
    calendar = Calendar(config['calendar_layer_token'])
    # logins = config.get('admins') or staff.list_users_by_department(config['admin_department'])
    duty_name, start, end = who_duty(calendar.get_now_events())
    print(duty_name)


def notify_action(args):
    config = args.config
    # staff = Staff(config['oauth_token'])
    calendar = Calendar(config['calendar_layer_token'])
    staff = Staff(config['oauth_token'])
    duty_name, start, end = who_duty(calendar.get_now_events())
    user_info = staff.get_users_data([duty_name])[0]
    announce(config, next_admin=user_info)


def notify_abc_action(args):
    config = args.config
    abc = ABC(config['abc_token'])
    staff = Staff(config['oauth_token'])
    duty_name = abc.get_current_duty(
        service_id=config.get('abc_service_id'),
        service_slug=config.get('abc_service_slug'),
        schedule_id=config.get('abc_schedule_id'),
        schedule_slug=config.get('abc_schedule_slug'),
    )
    user_info = staff.get_users_data([duty_name])[0]
    announce(config, next_admin=user_info)


def who_duty(events, start=None, end=None):
    next_admin = None
    events = events or []
    start, end = start or arrow.now(), end or arrow.now()
    for event in events:
        event_name, event_start, event_end = \
            event['name'], arrow.get(event['start']), arrow.get(event['end'])
        if range_intersect([event_start, event_end], [start, end]) > timedelta():
            m = re.search(r'(?P<name>[-\w]+)@', event_name)
            next_admin = m.group(b'name')
            break
    else:
        event_start, event_end = None, None
    return next_admin, event_start, event_end

def duty_rename(args):
    start, end, config = args.start, args.end, args.config
    calendar = Calendar(config['calendar_layer_token'])
    abc = ABC(token=config['abc_token'], service_id=config['abc_service_id'], schedule_id=config['abc_schedule_id'])
    for event in calendar.get_events(start, end):
        m = re.search(r'(?P<login>[-\w]+)@', event['name'])
        if m:
            name = "{}@ {} duty".format(m.group(b'login'), config['team'])
            calendar.delete_event(event['id'], m.group(b'login'))
            event_id = calendar.create_event(title, arrow.get(event['start']), arrow.get(event['end']), 'script generated')
            calendar.share_event(event_id, '{}@yandex-team.ru'.format(m.group(b'login')))
            abc.sync_shift(next_admin, arrow.get(event['start']), arrow.get(event['end']))

def share_with_abc(abc, calendar, start, end):
    for event in calendar.get_events(start, end):
        m = re.search(r'(?P<login>[-\w]+)@', event['name'])
        if m:
            abc.sync_shift(m.group(b'login'), arrow.get(event['start']), arrow.get(event['end']))

def check_duty_for_absent(config, calendar, range):
    log = logging.getLogger('absent-check')
    conditions = Conditions(config['oauth_token'],
                            config['conditions'],
                            arrow.now(),
                            arrow.now().replace(days=range))
    staff = Staff(config['oauth_token'])
    tz = config.get('tz', 'Europe/Moscow')
    start_hour = config.get('start_hour', 11)

    for start in arrow.Arrow.range('day', arrow.now(tz=tz).replace(hour=start_hour), limit=range):
        end = start.replace(days=1)
        if conditions.is_duty_day(start, end):
            gen_events = (event for event in calendar.get_events(start, end) if range_intersect([arrow.get(event['start']), arrow.get(event['end'])], [start, start.replace(seconds=1)]) > timedelta())
            events = list(gen_events)
            if not events:
                log.info("Empty event found {}. I will generate duty for it".format(arrow.get(start)))
                events = build_calendar_func(config, start, start, cleanup=True, abc_sync=True)
            else:
                for event in events:
                    m = re.search(r'(?P<login>[-\w]+)@', event['name'])
                    if m:
                        #if not conditions.not_absent(m.group(b'login'), arrow.get(event['start']), arrow.get(event['end'])) or staff.get_users_data([m.group(b'login')])[0].get('official', {}).get('is_dismissed'):
                        if not conditions.not_absent(m.group(b'login'), arrow.get(event['start']), arrow.get(event['end'])):
                            log.info("Absent or dissmissed found {} {}. I will delete it and try to generate new one".format(m.group(b'login'), arrow.get(event['start'])))
                            events = build_calendar_func(config, event['start'], event['start'], cleanup=True, abc_sync=True)
                            if events:
                                message = "@{} You assigned to {} duty on {} automatically. Reason: previous assigned duty @{} will be absent on this day. if you can't be on duty, contact your manager.".format(next(iter(events)).get('login'), config.get('team'), arrow.get(event['start']).format('YYYY-MM-DD'), m.group(b'login'))
                                for nyan_config in config.get('nyan_bot', []):
                                    for chat_id in nyan_config['chat_id']:
                                        if nyan_config.get('absent_alert'):
                                            nyan_tg_send(chat_id, re.escape(message), nyan_config.get('parse_mode'))

def get_abc_responsibles(args):
    log = logging.getLogger('get_abc_responsibles')
    config, service_id, service_slug = args.config, args.service_id, args.service_slug
    abc = ABC(token=config['abc_token'], service_id=config['abc_service_id'], schedule_id=config['abc_schedule_id'])
    res = abc.get_responsibles(service_id, service_slug=service_slug)
    for user in res:
        print(user['person']['login'])

def rotate_duty(args):
    log = logging.getLogger('rotate-duty')
    config, abc_sync = args.config, args.abc_sync
    calendar = Calendar(config['calendar_layer_token'])
    
    try:
        check_duty_for_absent(config, calendar, 14)
    except Exception,e:
        log.warn("Error during absent check: {}. Check wasn't completed".format(str(e)))    

    if not config.get('startrek', None) \
     or not config.get('abc_token', None) \
     or not config.get('abc_service_id', None) \
     or not config.get('abc_schedule_id', None):
        log.warn("Some config sections aren't defined properly: startrek, abc_token, abc_service_id or abc_schedule_id. Rotate stopped.")
        return

    queue = config['startrek']['queue']
    abc = ABC(token=config['abc_token'], service_id=config['abc_service_id'], schedule_id=config['abc_schedule_id']) if abc_sync else None
    st = Startrek(config['startrek'])
    juggler_changes = False

    if abc_sync:
        try:
            share_with_abc(abc,calendar, arrow.now(), arrow.now().replace(days=+14))
        except:
            log.warn("Error during abc sync. Sync wasn't completed")

    if config.get('juggler_rules'):
        juggler_changes = rotate_juggler(args)

    events = calendar.get_now_events()
    next_duty, start, end = who_duty(events)
    if next_duty:
        next_admin = Staff(config['oauth_token']).get_users_data([next_duty])[0]
        yesterday_duty = abc.get_shifts(start.replace(days=-1), end.replace(days=-1)) if abc_sync else None
        context = {
            'next_admin': next_admin,
            'date': start.format('DD.MM.YYYY')
        }
        summary = config['startrek']['summary'].format(**context)
        tickets = list(st.search_issue(query="Queue: {} AND Summary: {}".format(queue, summary)))
    
        if not tickets:
            ticket = re.escape(st.create_ticket(next_admin))
        else:
            ticket = None
        if yesterday_duty:
            try:
                abc.cleanup_shifts(start.replace(days=-1), end.replace(days=-1))
                abc.update_calendar(show_in_staff=False)
                abc.update_calendar(show_in_staff=True)
            except:
                log.warn("Error during abc yesterday shift remove. Shift wasn't removed")
        if ticket or juggler_changes: 
            if config.get('nyan_bot'):
                if not ticket:
                    ticket = re.escape(tickets[0]['key'])
                for key in next_admin.keys():
                    if isinstance(next_admin[key],(str,unicode)):
                        next_admin[key] = re.escape(next_admin[key])
                announce(config, next_admin=next_admin, ticket=ticket, team=re.escape(config['team']))

def rotate_juggler(args):
    log = logging.getLogger('rotate-juggler')
    config, dry_run = args.config, args.dry_run
    dry_run_msg = '[DRY RUN]' if dry_run else ''
    calendar = Calendar(config['calendar_layer_token'])
    juggler = Juggler(config['juggler_oauth_token'])
 
    events = calendar.get_now_events()
    next_duty, start, end = who_duty(events)
    if next_duty is None:
        log.warn('{}Nobody on call! Use defaults'.format(dry_run_msg))

    log.info("{} on call from {} to {}".format(next_duty, start, end))

    has_changes = False
    for rule_id, rule_info in config['juggler_rules'].iteritems():
        current_rule_logins = juggler.get_current_logins(rule_id)
        default_logins = rule_info.get('default_logins', current_rule_logins)

        if next_duty and next_duty in default_logins:
            next_logins = [next_duty] + [l for l in default_logins if l != next_duty]
        else:
            next_logins = default_logins

        log.debug('{} {} -> {} for rule "{}"'.format(dry_run_msg,
                                                     current_rule_logins,
                                                     next_logins,
                                                     rule_id))
        if current_rule_logins != next_logins:
            has_changes = True
            log.warn('{}Change: {} -> {} for rule "{}"'.format(dry_run_msg,
                                                               current_rule_logins,
                                                               next_logins,
                                                               rule_id))
            if not dry_run:
                juggler.set_rule_logins(rule_id, next_logins)
        else:
            log.info('No changes needed for rule "{}"'.format(rule_id))
    return has_changes

def rotate(args):
    config, dry_run = args.config, args.dry_run
    dry_run_msg = '[DRY RUN]' if dry_run else ''
    staff = Staff(config['oauth_token'])
    telegraph = Telegraph(config['oauth_token'])
    golem = Golem(config['oauth_token'])
    calendar = Calendar(config['calendar_layer_token'])
    phone_prefix = config.get('phone_prefix', '')
    logins = config.get('admins') or staff.list_users_by_department(config['admin_department'])
    admins = staff.get_users_data(logins)

    current_phone = telegraph.get_forward(config['robot_phone'])
    current_admin = admin_by_phone(admins, current_phone)
    events = calendar.get_now_events()
    duty_name, start, end = who_duty(events)
    next_admin = next(admin for admin in admins if admin['login'] == duty_name) if duty_name else None
    log.info("{} on call from {} to {}".format(next_admin, start, end))

    next_phone = '{}{}'.format(phone_prefix, next_admin['work_phone']) if next_admin else None

    if next_admin is None:
        log.warn('{}Nobody on call! Set {} as last responsible'.format(dry_run_msg,
                                                                       config['robot_name']))
        pos = 'last'
    else:
        pos = 'first'
    if not dry_run:
        golem.move_resp_to_position(config['robot_name'], pos)

    log.debug('{}{}({}) -> {}({})'.format(dry_run_msg,
                                          current_admin,
                                          current_phone,
                                          next_admin,
                                          next_phone))

    if next_phone != current_phone:
        log.warn('{}Change: {}({}) -> {}({})'.format(dry_run_msg,
                                                     current_admin,
                                                     current_phone,
                                                     next_admin,
                                                     next_phone))
        if not dry_run:
            telegraph.set_forward(config['robot_phone'], next_phone)
            announce(config, current_admin=current_admin, next_admin=next_admin)
    else:
        log.info('No changes needed')


def announce(config, next_admin=None, current_admin=None, ticket=None, team=None):
    context = {
        'next_admin': next_admin or nobody_dict,
        'current_admin': current_admin or nobody_dict,
        'ticket': ticket or 'нет тикета',
        'team': team or ''
    }
    for telegram_config in config.get('telegram', []):
        for chat_id in telegram_config['chat_id']:
            tg_send(telegram_config['token'], chat_id, telegram_config['message'].format(**context))

    for nyan_config in config.get('nyan_bot', []):
        for chat_id in nyan_config['chat_id']:
            nyan_tg_send(chat_id, nyan_config['message'].format(**context), nyan_config.get('parse_mode'))

    if 'console' in config:
        print(config['console']['message'].format(**context))


def range_intersect(a, b):
    return min(b[1], a[1]) - max(b[0], a[0])


def admin_for_duty(conditions, non_duties_counter):
    next_admin = non_duties_counter.most_common()[-1][0]
    # review detector :)
    for admin in non_duties_counter:
        if admin.startswith('u'):
            non_duties_counter[admin] -= 1
    # prev_admin = non_duties_counter.most_common()[-1][0]
    while True:
        d_start, d_end = yield next_admin
        log.debug('Look for next_admin for {} - {}'.format(d_start, d_end))
        log.debug('Non duties counter: {}'.format(non_duties_counter))
        day_weights = Counter(non_duties_counter)
        for admin in non_duties_counter:
            day_weights[admin] += conditions.suitable(admin, d_start, d_end)
        log.debug('Day weights: {}'.format(day_weights))
        next_admin = day_weights.most_common(1)[0][0]
        non_duties_counter[next_admin] -= len(non_duties_counter)
        for admin in non_duties_counter:
            if non_duties_counter[admin] < len(non_duties_counter):
                non_duties_counter[admin] += 1

        if next_admin is None:
            log.error('There is no next admin! {}'.format(non_duties_counter))
            break


def get_range(start, end, start_hour, tz):
    shift = {'hours': start_hour, 'minutes': 0, 'seconds': 0, 'microseconds': 0}
    if start is None:
        start = arrow.now(tz=tz).replace(months=1).floor('month')  # next month
    start = arrow.get(start).replace(tzinfo=tz).floor('day')
    if end is None:
        end = start.ceil('month')
    end = arrow.get(end).replace(tzinfo=tz).ceil('day')

    start, end = start.replace(**shift), end.replace(**shift)
    return start, end


def history_non_duties(admins, config, history_events):
    non_duties_counter = Counter({a: 0 for a in admins})

    duty_events_count = 0
    seen_duty = set()
    for event in reversed(history_events):  # assume sorted
        if duty_events_count >= len(admins):
            break
        m = re.search(r'(?P<login>[-\w]+)@', event['name'])
        if m:  # is duty event
            duty_admin = m.group(b'login')
            log.debug('Found duty event: {} {} - {}'.format(event['name'],
                                                            event['start'],
                                                            event['end']))
            duty_events_count += 1
            for admin in admins:
                if admin in seen_duty:
                    continue
                elif admin == duty_admin:
                    seen_duty.add(admin)
                    log.debug('Skip penalty for {} due to == duty_admin'.format(admin))
                else:
                    log.debug('Add penalty for {}'.format(admin))
                    non_duties_counter[admin] += 1

    for admin in admins:
        if admin not in seen_duty:
            non_duties_counter[admin] = len(admins)

    return non_duties_counter


def make_fake_event(event_name, start, end):
    return {
        "description": "fake",
        "start": arrow.get(start).isoformat(),
        "end": arrow.get(end).isoformat(),
        "id": "-1",
        "location": "",
        "name": event_name,
        "participants": [],
    }

def duty_close_ticket(st, issue):
    st.comment_issue(issue,text="This ticket was closed by robot-storage-duty because duty was over and all linked support tickets were solved.")
    log.info("Close ticket {} because all support linked tickets were solved".format(issue))
    st.workflow_issue(issue,'close',resolution="fixed")

def duty_telegram(config, need_report, closed):
    message = "{} queue ticket report:".format(config['duty_queue'])
    if not list(need_report):
        message += "\n\n\tGreat! All {} tickets were closed in time.".format(config['duty_queue'])
    else:
        message += "\n\n\tSome of the {} tickets are still not closed:".format(config['duty_queue'])
        for admin,c in need_report.most_common():
            message += "\n\t\t{}: {} ticket(s)".format(admin,c)
    if closed:
        message += "\n\n\t {} ticket(s) was/were closed automatically.".format(closed)
    for chat_id in config['chat_id']:
        nyan_tg_send(chat_id, message, config.get('parse_mode'))

def duty_report(args):
    config, duty_queue, support_queue = args.config, args.config['duty_report']['duty_queue'], args.config['duty_report']['support_queue']
    st = Startrek(config['startrek'])
    closed = 0
    ticket_count = 0
    need_report = Counter()
    for issue in st.search_issue(query="Created: <today()-1d AND Queue: {} AND Resolution: empty()".format(duty_queue),expand=["links"]): 
        if issue.get('links'):
            queries = []
            for link in issue.get('links'):
                queries.append("Key:{}".format(link['object']['key']))
            open_links = st.search_issue(query="(" + " OR ".join(queries) + ")" + " AND Resolution: empty() AND Queue: {}".format(support_queue))
            if list(open_links):
                need_report[issue['assignee']['id']] += 1
            else:
                duty_close_ticket(st,issue['key'])
                closed += 1
        else:
            duty_close_ticket(st,issue['key'])
            closed += 1
        ticket_count += 1
    log.info("{} open issues found in {} queue".format(ticket_count,duty_queue))
    duty_telegram(config['duty_report'],need_report, closed)

def abc_sync(args):
    log = logging.getLogger('abc-sync')
    duration, config = args.duration, args.config
    abc = ABC(token=config['abc_token'], service_id=config['abc_service_id'], schedule_id=config['abc_schedule_id'])
    calendar = Calendar(config['calendar_layer_token'])

    try:
        check_duty_for_absent(config, calendar, duration)
    except:
        log.warn("Error during absent check. Check wasn't completed") 
    
    share_with_abc(abc,calendar, arrow.now(), arrow.now().replace(days=duration))

# FIXME: 1d duty period assumptions everywhere
def build_calendar(args):
    config, start, end, last_duty, start_hour, dry_run, cleanup, share, abc_sync =\
        args.config, args.start, args.end, args.last_duty, \
        args.start_hour, args.dry_run, args.cleanup, args.share, args.abc_sync

    build_calendar_func(config, start, end, start_hour, last_duty, dry_run, cleanup, share, abc_sync)

def build_calendar_func(config, start, end, start_hour=11, last_duty=None, dry_run=None, cleanup=None, share=None, abc_sync=None):
    log = logging.getLogger('calendar')
    dry_run_msg = '[DRY RUN]' if dry_run else ''
    duties_stats = Counter()
    start, end = get_range(start, end, start_hour, config['tz'])
    log.info('Build calendar from {} to {}'.format(start, end))

    calendar = Calendar(config['calendar_layer_token'])
    staff = Staff(config['oauth_token'])
    abc = ABC(token=config['abc_token'], service_id=config['abc_service_id'], schedule_id=config['abc_schedule_id']) if abc_sync else None
    if share:
        log.info('{}Share all events with participants from {} to {}'.format(dry_run_msg, start, end))
        if not dry_run:
            calendar.share(start, end)
            if abc:
                share_with_abc(abc, calendar, start, end)
        return

    admins = config.get('admins') \
        or staff.list_users_by_department(config['admin_department'])

    history_start, history_end = start.replace(days=-30), start  # FIXME: magic number
    history_events = calendar.get_events(history_start, history_end)

    non_duties_counter = history_non_duties(admins, config, history_events)
    log.info('Starting non_duties_counter: {}'.format(non_duties_counter))

    conditions = Conditions(config['oauth_token'],
                            config['conditions'],
                            start,
                            end,
                            history_events)

    admins_getter = admin_for_duty(conditions, non_duties_counter)
    admins_getter.send(None)

    if cleanup:
        log.info('{}Delete all events from {} to {}'.format(dry_run_msg, start, end))
        if not dry_run:
            calendar.clean(start, end)
            if abc:
                abc.cleanup_shifts(start, end)

    events = []
    for d_start in arrow.Arrow.range('day', start, end):
        d_end = d_start.replace(days=1)
        if not conditions.is_duty_day(d_start, d_end):
            continue
        next_admin = admins_getter.send((d_start, d_end))
        duties_stats[next_admin] += 1
        log.info('{}Create duty: {} {} ({} - {})'.format(
            dry_run_msg, d_start.format('MM-DD'), next_admin, d_start, d_end))
        title_string = config.get('title_string', 'duty')
        title = '{}@ {} {}'.format(next_admin, config['team'], title_string)
        history_events.append(make_fake_event(title, d_start, d_end))
        events.append({"login": next_admin, "start": d_start, "end": d_end})
        if not dry_run:
            event_id = calendar.create_event(title, d_start, d_end, 'script generated')
            calendar.share_event(event_id, '{}@yandex-team.ru'.format(next_admin))
            if abc:
                abc.sync_shift(next_admin, d_start, d_end)

    log.info('Duties counts: {}'.format(duties_stats))
    log.info('Non duties counter: {}'.format(non_duties_counter))
    return events


@contextmanager
def monrun_report(path):
    status = '0;ok'
    try:
        yield
    except Exception as e:
        log.exception('')
        status = '2;{}'.format(e)
    finally:
        status = status.encode('utf-8')
        if path == '-':
            sys.stdout.write(status)
        elif path is not None:
            with open(path, 'w') as f:
                f.write(status)


def tg_send(token, chat_id, message):
    url = 'https://api.telegram.org/bot{}/sendMessage'.format(token)
    data = {
        'chat_id': -abs(int(chat_id)),
        'text': message
    }
    r = requests.post(url, json=data)
    r.raise_for_status()
    return r.json()


def nyan_tg_send(chat_id, message, parse_mode=None):
    chat_id = -abs(int(chat_id))
    url = 'https://api.nbt.media.yandex.net/v2/proxy/telegram/raw/{}'.format(chat_id)
    data = {'text': message}
    if parse_mode is not None:
        data['parse_mode'] = parse_mode
    r = requests.post(url, json=data, verify=False)  # FIXME
    r.raise_for_status()
    # TODO: check if job actually finished
    # https://api.nbt.media.yandex.net/v2/rq/jobs/{job_id}
    return r.json()


def date(s):
    if not s:
        return None
    elif s[0] in ['+', '-']:
        return arrow.now().replace(days=int(s))
    else:
        return arrow.get(s)


def get_args():
    common_parser = argparse.ArgumentParser(add_help=False)
    common_parser.add_argument('-c', '--config',  type=load_config, default='/etc/yandex/duty-bot/config.yaml')
    common_parser.add_argument('-n', '--dry-run', action='store_true')
    common_parser.add_argument('--log-level',     type=str, default='INFO')
    common_parser.add_argument('--log-file',      type=str)
    common_parser.add_argument('--state-file',    type=str,
                               help='write 0;ok or 2;<error> for monrun to provided file if set. "-" for stdout')
    common_parser.add_argument('--sleep', type=int, default=0,
                               help='Sleep after execution to hold zk-flock :)')
    common_parser.add_argument('--abc-sync', action='store_true', help='Sync all events with ABC duty calendar')
    parser = argparse.ArgumentParser(parents=[common_parser])
    subparsers = parser.add_subparsers()

    build_calendar_parser = subparsers.add_parser(
        'calendar',
        parents=[common_parser],
        help='Generate "on call" calendar')
    build_calendar_parser.add_argument('--start', type=date, help='Create duties from given date. Next month by default')
    build_calendar_parser.add_argument('--end',   type=date)
    build_calendar_parser.add_argument('--last-duty', help='name of last duty admin. auto detected if omitted')
    build_calendar_parser.add_argument('--start-hour', default=11,
                                       type=int, help='Start on call at provided hour in day')
    build_calendar_parser.add_argument('--cleanup', action='store_true',
                                       help='Clean all events in calendar in given range before creating duties')
    build_calendar_parser.add_argument('--share', action='store_true',
                                       help='Share all events in calendar in given range with participants')
    build_calendar_parser.set_defaults(func=build_calendar)

    rotate_parser = subparsers.add_parser(
        'rotate',
        parents=[common_parser],
        help='Set robot forward to current duty admin')
    rotate_parser.set_defaults(func=rotate)

    rotate_juggler_parser = subparsers.add_parser(
        'rotate-juggler',
        parents=[common_parser],
        help='Set current duty admin as first user in juggler escalation')
    rotate_juggler_parser.set_defaults(func=rotate_juggler)

    rotate_duty_parser = subparsers.add_parser(
        'rotate-duty',
        parents=[common_parser],
        help='Switch duty to next admin')
    rotate_duty_parser.set_defaults(func=rotate_duty) 

    abc_sync_parser = subparsers.add_parser(
        'abc-sync',
        parents=[common_parser],
        help='Sync calendar with ABC')
    abc_sync_parser.set_defaults(func=abc_sync)
    abc_sync_parser.add_argument('--duration', default=15,
                                       type=int, help='Sync N days from today with ABC')

    get_abc_responsibles_parser = subparsers.add_parser(
        'get-abc-responsibles',
        parents=[common_parser],
        help='Get ABC service responsibles')
    get_abc_responsibles_parser.set_defaults(func=get_abc_responsibles)
    get_abc_responsibles_parser.add_argument('--service_id',type=int, help='ABC service id')
    get_abc_responsibles_parser.add_argument('--service_slug',type=str, help='ABC service slug')

    who_duty_parser = subparsers.add_parser(
        'whoduty',
        parents=[common_parser],
        help='Print login of "on duty" user')
    who_duty_parser.set_defaults(func=who_duty_action)

    notify_parser = subparsers.add_parser(
        'notify',
        parents=[common_parser],
        help='Execute notifications about current duty admin (like telegram)')
    notify_parser.set_defaults(func=notify_action)

    abc_notify_parser = subparsers.add_parser(
        'notify-abc',
        parents=[common_parser],
        help='Execute notifications about current duty admin from abc (like telegram)')
    abc_notify_parser.set_defaults(func=notify_abc_action)

    duty_report_parser = subparsers.add_parser(
        'duty-report',
        parents=[common_parser],
        help='Report all unresolved duty tickets to Telegram')
    duty_report_parser.set_defaults(func=duty_report)

    duty_rename_parser = subparsers.add_parser(
        'duty-rename',
        parents=[common_parser],
        help='Rename events')
    duty_rename_parser.add_argument('--start', type=date, help='Create duties from given date. Next month by default')
    duty_rename_parser.add_argument('--end',   type=date)
    duty_rename_parser.set_defaults(func=duty_rename)

    return parser.parse_args()


if __name__ == '__main__':
    args = get_args()
    params = {} if args.log_file is None else {'filename': args.log_file}
    logging.basicConfig(format='%(asctime)s %(name)s %(levelname)s: %(message)s',
                        level=args.log_level.upper(),
                        **params)
    logging.getLogger('TVM').setLevel("ERROR")
    logging.getLogger('requests').setLevel("ERROR")
    Calendar.tvm2_token = args.config['tvm2_token']
    with monrun_report(args.state_file):
        args.func(args)
    time.sleep(args.sleep)
