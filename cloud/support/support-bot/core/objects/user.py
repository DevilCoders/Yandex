#!/usr/bin/env python3
"""This module contains User and UserConfig classes."""

import logging
from datetime import datetime

from utils.config import Config
from core.objects.base import Base
from core.database import init_db_client
from core.constants import DUTY_STAFF, DEVELOPERS
from services.staff import StaffClient

logger = logging.getLogger(__name__)


class User(Base):
    """This object represents the telegram user.

    Arguments:
      id: int
      username: str
      staff_login: str
      is_allowed: bool
      checkout_permission: bool
      join_config: bool

      other arguments only for generate object from database

    Attributes:
      telegram_id: int
      telegram_login: str
      staff_login: str
      is_allowed: bool
      config: object

    """

    def __init__(self,
                 id=None,
                 telegram_id=None,
                 username=None,
                 telegram_login=None,
                 staff_login=None,
                 is_allowed=False,
                 is_support=None,
                 force_checkout=False,
                 join_config=True,
                 join_roles=True,
                 client=None,
                 **kwargs):

        self.telegram_id = id or telegram_id
        self.telegram_login = username.lower() if isinstance(username, str) else username or telegram_login
        self.staff_login = staff_login
        self.is_allowed = is_allowed
        self.is_support = is_support

        self._is_exist = False
        self._join_config = join_config
        self._join_roles = join_roles
        self._client = client
        self._force_checkout = force_checkout
        self._db_session = init_db_client

        if self.staff_login is None or self._force_checkout:
            self._local = self._checkout()
        else:
            self._local = self.to_dict()
            self._local.update(kwargs)

        logger.debug(f'Detailed UserData: {self.__dict__}')

    def _checkout(self):
        """Checkout user permissions and sync with database."""
        _general_keys = ('telegram_id', 'telegram_login', 'staff_login', 'is_allowed')
        keyfilter = lambda data: dict((k, v) for (k, v) in data.items() if k in _general_keys)

        local = self._db_session().get_user(self, with_config=self._join_config) or {}
        self._is_exist = False if not local else True

        if self._is_exist and self.staff_login is None:
            self.staff_login = local.get('staff_login')
            self.is_allowed = local.get('is_allowed')
            self.is_support = local.get('is_support')

        if self._force_checkout or (not self._is_exist and self.staff_login is None):
            staff = StaffClient(Config.STAFF_TOKEN).get_user(telegram=self.telegram_login)
            self.staff_login = staff.login
            self.is_allowed = staff.is_allowed
#            self.is_support = staff.is_support

        try:
            if self._is_exist:
                assert keyfilter(local) == self.to_dict()
            else:
                logger.debug('Userdata comparison is not required for a nonexistent (external) user')
        except AssertionError:
            self.update()
        finally:
            local.update(self.to_dict())
            return local

    @property
    def config(self):
        """Return and create user config."""
        local = self._local if self._join_config else self._db_session().get_user_config(self)
        return UserConfig.de_json(local)

    @property
    def roles(self):
        """Return and create user config with roles."""
        local = self._local if self._join_roles else self._db_session().get_user_role(self)
        print(self._db_session().get_user_role(self))
        return UserRoles.de_json(local)

    @classmethod
    def de_json(cls, data: dict, client: object = None, force_checkout=False):
        """Packages the dict into an object."""
        if not data:
            return None

        data['force_checkout'] = force_checkout
        data = super(User, cls).de_json(data, client)
        return cls(client=client, **data)

    @classmethod
    def de_list(cls, data: list, client: object = None, force_checkout=False):
        """Packages each dict in the list into an object."""
        if not data:
            return []

        users = list()
        for user in data:
            users.append(cls.de_json(user, client, force_checkout=force_checkout))
        return users

    def create(self):
        """Shortcut for database.create_user."""
        self._db_session().add_user(self)
        UserConfig.de_json(self.to_dict()).create()

    def update(self):
        """Shortcut for database.update_user."""
        return self._db_session().update_user(self)

    def delete(self):
        """Shortcut for database.delete_user."""
        return self._db_session().delete_user(self)

    def grant_admin(self):
        pass

    def revoke_admin(self):
        pass


class UserConfig(Base):
    """This object represents a user settings.

    Attributes:
      telegram_id: int
      is_admin: bool
      is_duty: bool
      cloudabuse: bool
      cloudsupport: bool
      premium_support: bool
      business_support: bool
      standard_support: bool
      ycloud: bool
      work_time: str
      notify_interval: int
      do_not_disturb: bool
      ignore_work_time: bool

    Compatibility args:
      staff_login: str (from database compatibility)

    """

    def __init__(self,
                 telegram_id=None,
                 staff_login=None,
                 is_admin=False,
                 is_duty=None,
                 cloudabuse=False,
                 cloudsupport=False,
                 premium_support=False,
                 business_support=False,
                 standard_support=False,
                 ycloud=False,
                 cloudops=False,
                 work_time='10-19',
                 notify_interval=10,
                 do_not_disturb=False,
                 ignore_work_time=False,
                 client=None,
                 **kwargs):

        self.telegram_id = telegram_id
        self.is_admin = True if telegram_id in DEVELOPERS else is_admin
        self.is_duty = is_duty or True if staff_login in DUTY_STAFF else False
        self.cloudabuse = cloudabuse
        self.cloudsupport = cloudsupport
        self.premium_support = premium_support
        self.business_support = business_support
        self.standard_support = standard_support
        self.ycloud = ycloud
        self.cloudops = cloudops
        self.work_time = work_time
        self.notify_interval = notify_interval
        self.do_not_disturb = do_not_disturb
        self.ignore_work_time = ignore_work_time

        self._client = client
        self._db_session = init_db_client

    @property
    def start_workday(self):
        """Return first hour of workday as int."""
        hour = self.work_time.split('-')[0]
        return int(hour)

    @property
    def end_workday(self):
        """Return last hour of workday as int."""
        hour = self.work_time.split('-')[1]
        return int(hour)

    @property
    def not_subscriber(self):
        """Checkout user subscriptions."""
        channels = ('ycloud', 'cloudsupport', 'cloudabuse',
                    'premium_support', 'business_support', 'standard_support')
        subs = lambda data: dict((key, val) for (key, val) in data.items() if key in channels and val)

        if not subs(self.to_dict()):
            return True
        return False

    @classmethod
    def de_json(cls, data: dict, client: object = None):
        """Packages the dict into an object."""
        if not data:
            return None

        data = super(UserConfig, cls).de_json(data, client)
        return cls(client=client, **data)

    def pull(self):
        """Force get user config and return."""
        return UserConfig.de_json(self._db_session().get_user(self, with_config=True))

    def create(self):
        """Shortcut for database.add_user_config."""
        return self._db_session().add_user_config(self)

    def update(self, key: str, value: str):
        """Shortcut for database.update_user_config."""
        return self._db_session().update_user_config(self, key, value)

    def delete(self):
        """Shortcut for database.delete_user_config."""
        return self._db_session().delete_user_config(self)


class UserRoles(Base):
    """This object represents a user settings.

    Attributes:
      telegram_id: int
     is_admin: bool
      is_duty: bool
      cloudabuse: bool
      cloudsupport: bool
      premium_support: bool
      business_support: bool
      standard_support: bool
      ycloud: bool
      work_time: str
      notify_interval: int
      do_not_disturb: bool
      ignore_work_time: bool

    Compatibility args:
      staff_login: str (from database compatibility)

    """

    def __init__(self,
                 staff_login=None,
                 monday=None,
                 tuesday=None,
                 wednesday=None,
                 thursday=None,
                 friday=None,
                 saturday=None,
                 sunday=None,
                 client=None,
                 **kwargs):

        self.monday = monday
        self.tuesday = tuesday
        self.wednesday = wednesday
        self.thursday = thursday
        self.friday = friday
        self.saturday = saturday
        self.sunday = sunday

        self._client = client
        self._db_session = init_db_client


    def pull(self):
        """Force get user config and return."""
        return UserRoles.de_json(self._db_session().get_user(self, with_roles=True))
    
    @classmethod
    def de_json(cls, data: dict, client: object = None):
        """Packages the dict into an object."""
        if not data:
            return None
        
        data = super(UserRoles, cls).de_json(data, client)
        return cls(client=client, **data)


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

        self.person_login = person_login
        self.gaps = gaps

        self._other = kwargs

        print('ИНИЦИАЛИЗАЦИЯ UserGap__________________________________')
        print(self)

    #@classmethod
    #def de_json(cls, data: dict, client: object = None):
    #    """Packages the dict into an object."""
    #    if not data:
    #        return None#

    #    data = super(UserGap, cls).de_json(data, client)
    #    return cls(client=client, **data)

def passed():
    pass    
