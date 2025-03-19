#!/usr/bin/env python3
"""This module contains ABC duty 2.0 class."""

import logging
import tvmauth
import socket
import datetime

from core.objects.base import Base
from core.error import LogicError, NotFound, StaffError
from core.objects.employee import YandexEmployee
from core.constants import DEFAULT_TIMEOUT
from utils.config import Config
from utils.request import Request

logger = logging.getLogger(__name__)


class Shift(Base):
    """This object represents a abc2 shift.

    Attributes:
      gaps: list

    Compatibility args:
      person_login: str (from database compatibility)

    """

    def __init__(self,
                 slot_id=None,
                 start=None,
                 end=None,
                 staff_login=None,
                 is_primary=None,
                 **kwargs):
        if slot_id is not None:
            self.slot_id = slot_id
        self.start = start.isoformat("T") + "Z"
        self.end = end.isoformat("T") + "Z"
        self.approved = False
        self.empty = True if not staff_login else False
        self.staff_login = staff_login
        self.is_primary = is_primary
        self._other = kwargs

#        print('ИНИЦИАЛИЗАЦИЯ UserGap__________________________________')
#        print(self)
#        print('ВЫБОРКА________________________________________________')
#        print('_______________________________________________________')
#        for i in gaps:
#            print(i['person_login'])
#            print(i['date_from'])
#            print(i['date_to'])
#        print('_______________________________________________________')
#        print('_______________________________________________________')
#        print('_______________________________________________________')

    @classmethod
    def de_json(cls, data: dict, client: object = None):
        """Packages the dict into an object."""
        if not data:
            return None#

        data = super(UserGap, cls).de_json(data, client)
        return cls(client=client, **data)

class Abc2Client(Base):
    """This object provides an interface for working with the ABC duty 2.0.

    If args not specified – using from config or default values.

    Arguments:
      token: str
      endpoint: str
      request: object
      timeout: int

    """

    def __init__(self,
                 token=None,
                 tvm_token=None,
                 endpoint=None,
                 bb_api=None,
                 #request=None,
                 timeout=None,
                 tvmclient=None,
                 **kwargs):

        self.token = token or Config.STAFF_TOKEN
        self.tvm_token = tvm_token or Config.TVM_LOCAL_TOKEN
        self.endpoint = endpoint or Config.ABC2_ENDPOINT
        self.bb_api = bb_api or Config.BLACKBOX_ENDPOINT
        self.timeout = timeout or DEFAULT_TIMEOUT
        self._other = kwargs
    
        if self.token is None:
            raise ValueError('Token for OAuth is empty.')

        #if request:
        #    self._request = request
        #    self._request.set_and_return_client(self)
        #else:
        self._request = Request(self, timeout=self.timeout, token_type='TVM')
        
        try:
            self.tvmclient = tvmauth.TvmClient(
                tvmauth.TvmToolClientSettings(
                    self_alias="bot",
                    auth_token=self.tvm_token,
                    port=18080)
            )
        except Exception as error:
                logging.error('TVM Client failed: {error}')
                
        try: 
            s = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
            s.connect(('yandex-team.ru', 1))
            self.localip = s.getsockname()[0]
            
            s.close()
            
        except Exception as error:
            logging.error('Error parsing local IPv6 address: {error}')
            
        #exitlogging.info(f'abc init end: {self}')

    def __del__(self):
        if self.tvmclient:
            self.tvmclient.stop()
        
    def _get_service_ticket(self):
        if self.tvmclient:
            return self.tvmclient.get_service_ticket_for(alias="abc2")
        else:
            return None
    
    def _get_user_ticket(self):
        service_ticket = self.tvmclient.get_service_ticket_for(alias="bb")
        headers= {'X-Ya-Service-Ticket': f'{service_ticket}'}
        
        user_ticket_request = Request(self, headers=headers, endpoint=self.bb_api, timeout=self.timeout)
        user_ticket_request.set_authorization(self.token)
        
        
        url = f'{self.bb_api}/blackbox?method=oauth&get_user_ticket=yes&userip={self.localip}&format=json'
        try:
            response = user_ticket_request.get(url)
            #logging.info(f'Log blackbox response: {response}')
            return Base.de_json(response, self)['user_ticket']
        except Exception as err:
            logging.error(f'User Ticket get error: {err}')
        return None
        
    def enable(self, schedule_id, enable = True):
        #if schedule_id is None:
        #    raise LogicError("Staff login and telegram login can't be passed together.")

        if schedule_id is None:
            raise ValueError('Schedule id must be passed')

        params = {
            'state': 'active' if enable else 'disabled',
        }
        
        self._request.set_authorization(token='', service_ticket=self._get_service_ticket(), user_ticket=self._get_user_ticket())

#        if login:
#            params['login'] = login
#        elif telegram:
#            params['accounts.value_lower'] = telegram.lower()

        url = f'{self.endpoint}/api/watcher/v1/schedule/{schedule_id}'
        try:
            response = self._request.patch(url, json=params)
            logging.debug(f'Log abc2 response: {response}')
            return Base.de_json(response, self)
        except Exception as err:
            logging.warning(f'ABC2 api error: {err}')
        return None

    def get_schedule(self, schedule_id: str = None):
        if schedule_id is None:
            raise ValueError('Schedule id must be passed')
        
        self._request.set_authorization(token='', service_ticket=self._get_service_ticket(), user_ticket=self._get_user_ticket())


        url = f'{self.endpoint}/api/watcher/v1/schedule/{schedule_id}'
        try:
            response = self._request.get(url)
            logging.info(f'Log abc2 response: {response}')
            return Base.de_json(response, self)
        except Exception as err:
            logging.warning(f'ABC2 api error: {err}')
        return None

    def get_curr_version(self, schedule_id: str = None):
        if schedule_id is None:
            raise ValueError('Schedule id must be passed')
        
        self._request.set_authorization(token='', service_ticket=self._get_service_ticket(), user_ticket=self._get_user_ticket())


        url = f'{self.endpoint}/api/watcher/v1/revision/current-revision?schedule_id={schedule_id}'
        try:
            response = self._request.get(url)
            logging.info(f'Log abc2 response: {response}')
            return Base.de_json(response, self)
        except Exception as err:
            logging.warning(f'ABC2 api error: {err}')
        return None

    def get_slot_ids(self, schedule_id: str = None):
        if schedule_id is None:
            raise ValueError('Schedule id must be passed')
        
        self._request.set_authorization(token='', service_ticket=self._get_service_ticket(), user_ticket=self._get_user_ticket())


        url = f'{self.endpoint}/api/watcher/v1/revision/current-revision?schedule_id={schedule_id}'
        try:
            response = self._request.get(url)
            logging.info(f'Log abc2 response: {response}')
            resp = Base.de_json(response, self)
            slots = []
            for a in [x.get('slots') for x in resp.get('intervals')]:
                for b in a:
                    slots.append(b.get('id'))
            return slots
        except Exception as err:
            logging.warning(f'ABC2 api error: {err}')
        return None
    
    def cancel_recalc(self, schedule_id: str = None, recalc = False):
        if schedule_id is None:
            raise ValueError('Schedule id must be passed')
        
        self._request.set_authorization(token='', service_ticket=self._get_service_ticket(), user_ticket=self._get_user_ticket())

        data = {
          'recalculate': recalc
        }

        url = f'{self.endpoint}/api/watcher/v1/schedule/{schedule_id}'
        try:
            response = self._request.patch(url, json=data)
            logging.info(f'Log abc2 response: {response}')
            return Base.de_json(response, self)
        except Exception as err:
            logging.warning(f'ABC2 api error: {err}')
        return None

    def recalculate(self, schedule_id: str = None):
        if schedule_id is None:
            raise ValueError('Schedule id must be passed')

        self._request.set_authorization(token='', service_ticket=self._get_service_ticket(), user_ticket=self._get_user_ticket())


        url = f'{self.endpoint}/api/watcher/v1/schedule/{schedule_id}/recalculate'
        try:
            response = self._request.post(url)
            logging.info(f'Log abc2 response: {response}')
            return Base.de_json(response, self)
        except Exception as err:
            logging.warning(f'ABC2 api error: {err}')
        return None
    
    def get_shifts(self, schedule_id: str = None):
        if schedule_id is None:
            raise ValueError('Schedule id must be passed')
        
        self._request.set_authorization(token='', service_ticket=self._get_service_ticket(), user_ticket=self._get_user_ticket())
        
        params = {
            'filter': f'schedule_id={schedule_id}'
        }

        url = f'{self.endpoint}/api/watcher/v1/shift'
        try:
            response = self._request.get(url, json=params)
            logging.info(f'Log abc2 response: {response}')
            return Base.de_json(response, self)
        except Exception as err:
            logging.warning(f'ABC2 api error: {err}')
        return None

    def patch_shift(self, shift_id: str = None, empty = False):
        if shift_id is None:
            raise ValueError('Shift id must be passed')
        
        self._request.set_authorization(token='', service_ticket=self._get_service_ticket(), user_ticket=self._get_user_ticket())

        data = {
          'empty': empty
        }

        url = f'{self.endpoint}/api/watcher/v1/shift/{shift_id}'
        try:
            response = self._request.patch(url, json=data)
            logging.info(f'Log abc2 response: {response}')
            return Base.de_json(response, self)
        except Exception as err:
            logging.warning(f'ABC2 api error: {err}')
        return None
    
    def upload_shifts(self, schedule_id = None, shifts = None, replace = True):
        if schedule_id is None:
            raise ValueError('Schedule id must be passed')


        if schedule_id is None:
            shifts = []
#            shifts.append(Shift(slot_id=1,
#                            start=datetime.datetime.combine(datetime.date.today(), datetime.time(7)),
#                            end=datetime.datetime.combine(datetime.date.today(), datetime.time(19)),
#                            staff_login="nooss",
#                            is_primary=True).to_dict()
#                     )
#            shifts.append(Shift(slot_id=1,
#                            start=datetime.datetime.combine(datetime.date.today()+datetime.timedelta(days=1), datetime.time(7)),
#                            end=datetime.datetime.combine(datetime.date.today()+datetime.timedelta(days=1), datetime.time(19)),
#                            staff_login="nooss2",
#                            is_primary=False).to_dict()
#                     )

        data = {
          'shifts': shifts,
          'replace': replace,
          'schedule_id': schedule_id,
        }
        
        self._request.set_authorization(token='', service_ticket=self._get_service_ticket(), user_ticket=self._get_user_ticket())


        url = f'{self.endpoint}/api/watcher/v1/shift/upload'
        try:
            response = self._request.post(url, json=data)
            logging.info(f'Log abc2 response: {response}')
            return Base.de_json(response, self)
        except Exception as err:
            logging.warning(f'ABC2 api error: {err}')
        return None


#    def get_vacation(self, person_login='vr-owl', since='2020-01-01', until='2020-12-31'):
#        """Return oncall list. Since, until format: 1970-01-01"""
#        print('НАЧАЛО ИНИЦИАЛИЗАЦИИ get_vacation___________________________________________________________')
#        url = f'{self.api_v3}/gap-api/api/gaps_find/?person_login={person_login}&workflow=vacation&date_from={since}&date_to={until}'
#        print('URL get_vacation______________________________________________________________')
#        print(url)
#        try:
#            response = self._request.get(url, verify=False)
#            print(response)
#            return UserGap.de_json(response, self)
#        except Exception as error:
#            raise StaffError(error)
