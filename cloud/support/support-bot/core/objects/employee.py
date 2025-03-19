#!/usr/bin/env python3
"""This module contains YandexEmployee class."""

from core.objects.base import Base
from core.constants import CLOUD_DEPARTMENT, SUPPORT_DEPARTMENT


class YandexEmployee(Base):
    """This object represents a Yandex employee.

    Arguments:
      login: str
      department_group: dict
      accounts: list
      official: dict

    Attributes:
      department: str
      login: str
      telegram_login: str
      is_dismissed: bool
      is_allowed: bool
      is_cloud_staff: bool

    """

    def __init__(self,
                 login=None,
                 department_group=None,
                 accounts=None,
                 official=None,
                 client=None,
                 **kwargs):

        self.department = department_group.get('parent', {}).get('url') if isinstance(department_group, dict) else None
        self._accounts = accounts or []

        self.login = login
        self.telegram_login = self.parse_telegram()
        self.is_dismissed = official.get('is_dismissed', True) if isinstance(official, dict) else True
        self.is_allowed = self.is_cloud_staff

        self._client = client

    @property
    def is_cloud_staff(self):
        """Return True if employee from Yandex.Cloud department."""
        if self.department is None or self.is_dismissed:
            return False

        if self.department.startswith(CLOUD_DEPARTMENT):
            return True
        return False

    @property
    def is_support(self):
        """Return True if employee is member of YC Support."""
        if self.department == SUPPORT_DEPARTMENT:
            return True
        return False

    def parse_telegram(self):
        """Get telegram login from user accounts."""
        if not self._accounts:
            return

        for account in self._accounts:
            if account.get('type') == 'telegram':
                return account.get('value_lower')
        return

    @classmethod
    def de_json(cls, data: dict, client: object = None):
        """Packages the dict into an object."""
        if not data:
            return None

        data = super(YandexEmployee, cls).de_json(data, client)
        return cls(client=client, **data)
