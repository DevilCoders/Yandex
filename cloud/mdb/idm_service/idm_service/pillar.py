"""Pillar modification logic."""
import datetime
import logging
import time

from .exceptions import InvalidUserOriginError, NotImplementedError
from .utils import encrypt, generate_password, IDM_ORIGIN

# pylint: disable=invalid-name
logger = logging.getLogger(__name__)


class Pillar:
    """An interface for Pillar modification."""

    def __init__(self, data):
        self.data = data
        self.validate()

    def validate(self):
        raise NotImplementedError()

    @property
    def users(self):
        raise NotImplementedError()

    def databases(self):
        raise NotImplementedError()

    def get_default_user(self):
        raise NotImplementedError()

    def add_user_role(self, user, role):
        raise NotImplementedError()

    def remove_user_role(self, user, role):
        raise NotImplementedError()

    def get_user_external_roles(self, user):
        """
        Get user roles for external system, or None
        """
        raise NotImplementedError()

    @property
    def resps(self):
        """Get responsible for cluster"""
        if 'idm_resps' not in self.data['data']:
            self.data['data']['idm_resps'] = list()
        return self.data['data']['idm_resps']

    def add_resp(self, login):
        """Add responsible for cluster. Return False if no action was taken"""
        if login in self.resps:
            return False
        if self.resps is None:
            self.resps = list()
        self.resps.append(login)
        return True

    def remove_resp(self, login):
        """Remove responsible for cluster. Return False if no action was taken"""
        if login not in self.resps:
            return False
        self.resps.remove(login)
        return True

    def add_role(self, login, role):
        """Add role to the user. Create new user if needed.
        Return False if no action was taken.
        """
        if login not in self.users:
            user = self.get_default_user()
            self.users[login] = user
        else:
            user = self.users[login]
            if user.get('origin') != IDM_ORIGIN:
                raise InvalidUserOriginError(login)
        self.add_user_role(user, role)
        return True

    def remove_role(self, login, role):
        """Remove role from the user.
        Return False if no action was taken.
        """
        if login not in self.users:
            return False
        user = self.users[login]
        self.remove_user_role(user, role)
        return True

    def get_idm_users(self):
        result = {}
        for user_name, data in self.users.items():
            if data.get('origin') == 'idm':
                ext_roles = self.get_user_external_roles(data)
                if ext_roles:
                    result[user_name] = ext_roles

        for user in self.resps:
            if user not in result:
                result[user] = []
            result[user].append('responsible')

        return result

    def have_stale_passwords(self, pass_lifespan=90):
        """Return list of users with stale passwords."""
        result = []
        lifespan = datetime.timedelta(days=pass_lifespan).total_seconds()
        for login, info in self.users.items():
            if info.get('origin', 'other') != IDM_ORIGIN:
                continue
            if info.get('last_password_update', 0) + lifespan < time.time():
                result.append(login)
        return result

    def rotate_passwords(self, crypto, pass_lifespan=90):
        """Rotate stale passwords.
        Return dict with new passwords like:
            {
                'user1': 'new_password1',
                'user2': 'new_password2',
                ...
            }
        """
        need_pass_update = self.have_stale_passwords(pass_lifespan)
        new_passwords = {}
        for login in need_pass_update:
            user = self.users[login]
            new_pass = generate_password()
            encrypted = encrypt(crypto, new_pass)
            user['password'] = encrypted
            user['last_password_update'] = int(time.time())
            new_passwords[login] = new_pass
        return new_passwords
