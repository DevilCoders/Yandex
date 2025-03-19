#!/usr/bin/env python3
"""This module contains StaffClient class."""

import logging

from core.objects.base import Base
from core.error import LogicError, NotFound, StaffError
from core.objects.employee import YandexEmployee
from core.constants import DEFAULT_TIMEOUT
from utils.config import Config
from utils.request import Request

logger = logging.getLogger(__name__)


class UserGap(Base):
    """This object represents a user gaps.

    Attributes:
      gaps: list

    Compatibility args:
      person_login: str (from database compatibility)

    """

    def __init__(self,
                 person_login=None,
                 gaps=None,
                 **kwargs):

        self.gaps = gaps
        
        self._other = kwargs

        print('ИНИЦИАЛИЗАЦИЯ UserGap__________________________________')
        print(self)
        print('ВЫБОРКА________________________________________________')
        print('_______________________________________________________')
        for i in gaps:
            print(i['person_login'])
            print(i['date_from'])
            print(i['date_to'])
        print('_______________________________________________________')
        print('_______________________________________________________')
        print('_______________________________________________________')

    @classmethod
    def de_json(cls, data: dict, client: object = None):
        """Packages the dict into an object."""
        if not data:
            return None#

        data = super(UserGap, cls).de_json(data, client)
        return cls(client=client, **data)

class StaffClient(Base):
    """This object provides an interface for working with the Staff.

    If args not specified – using from config or default values.

    Arguments:
      token: str
      endpoint: str
      request: object
      timeout: int

    """

    def __init__(self,
                 token=None,
                 endpoint=None,
                 api_v3=None,
                 request=None,
                 timeout=None,
                 **kwargs):

        self.token = token or Config.STAFF_TOKEN
        self.endpoint = endpoint or Config.STAFF_ENDPOINT
        self.api_v3 = 'https://staff.yandex-team.ru'
        self.timeout = timeout or DEFAULT_TIMEOUT
        self._other = kwargs
    
        if self.token is None:
            raise ValueError('Token for StaffClient is empty.')

        if request:
            self._request = request
            self._request.set_and_return_client(self)
        else:
            self._request = Request(self, timeout=self.timeout)

    def get_user(self, login: str = None, telegram: str = None):
        if telegram and login:
            raise LogicError("Staff login and telegram login can't be passed together.")

        if telegram is None and login is None:
            raise ValueError('Staff login or telegram login must be passed')

        params = {
            '_one': '1',
#            '_fields': 'login,official.is_dismissed,department_group.parent.url,accounts',
            '_fields': 'login,official.is_dismissed,department_group.parent.url',
        }

        if login:
            params['login'] = login
        elif telegram:
            params['telegram_accounts.value_lower'] = telegram.lower()

        url = f'{self.endpoint}/v3/persons?'
        try:
            response = self._request.get(url, params=params)
            return YandexEmployee.de_json(response, self)
        except NotFound as err:
            logging.warning(f'Employee with login: {login}, telegram: {telegram} not found')
        return YandexEmployee()


    def get_vacation(self, person_login='vr-owl', since='2020-01-01', until='2020-12-31'):
        """Return oncall list. Since, until format: 1970-01-01"""
        print('НАЧАЛО ИНИЦИАЛИЗАЦИИ get_vacation___________________________________________________________')
        url = f'{self.api_v3}/gap-api/api/gaps_find/?person_login={person_login}&workflow=vacation&date_from={since}&date_to={until}'
        print('URL get_vacation______________________________________________________________')
        print(url)
        try:
            response = self._request.get(url, verify=False)
            print(response)
            return UserGap.de_json(response, self)
        except Exception as error:
            raise StaffError(error)