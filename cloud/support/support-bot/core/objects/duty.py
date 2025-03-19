#!/usr/bin/env python3
"""This module contains YandexCloudDuty and Resp class."""

from datetime import timedelta

from core.objects.base import Base
from core.constants import HOUR
from utils.helpers import de_timestamp


class YandexCloudDuty(Base):
    """This object represents a duty from GoRe.

    Arguments:
      shift: bool (-2 hours time shift for support team)
      datestart: int
      dateend: int
      resp: dict

    Attributes:
      datestart: int
      dateend: int
      resp: object
      start_datetime: datetime
      end_datetime: datetime

    """

    def __init__(self,
                 shift=True,
                 datestart=None,
                 dateend=None,
                 resp=None,
                 **kwargs):

        self.shift = shift
        self.datestart = self._timeshift(datestart)
        self.dateend = self._timeshift(dateend)
        self.resp = resp
        self._other = kwargs

    @property
    def start_datetime(self):
        if isinstance(self.datestart, int):
            return de_timestamp(self.datestart)
        return self.datestart

    @property
    def end_datetime(self):
        if isinstance(self.dateend, int):
            return de_timestamp(self.dateend)
        return self.dateend

    def _timeshift(self, dtime):
        """Time shift for YC Support team."""
        if isinstance(dtime, int) and self.shift:
            return dtime
        return dtime

    @classmethod
    def de_json(cls, data: dict, client: object = None, shift=True):
        """Packages the dict into an object."""
        if not data:
            return None

        data['resp'] = Resp.de_json(data.get('resp'), client)
        data['shift'] = shift
        data = super(YandexCloudDuty, cls).de_json(data, client)
        return cls(client=client, **data)

    @classmethod
    def de_list(cls, data: list, client: object = None, shift=True):
        """Packages each dict in the list into an object."""
        if not data:
            return []

        duty_list = list()
        for duty in data:
            duty_list.append(cls.de_json(duty, client, shift=shift))
        return duty_list


class Resp(Base):
    """This object represents a resp from GoRe.

    Arguments:
      order: int
      username: str

    Attributes:
      order: int
      username: str
      primary: bool
      backup: bool

    """

    def __init__(self,
                 order=None,
                 username=None,
                 **kwargs):

        self.order = order
        self.username = username
        self.primary = False if order != 0 else True
        self.backup = True if order > 0 else False

        self._other = kwargs

    @classmethod
    def de_json(cls, data: dict, client: object = None):
        """Packages the dict into an object."""
        if not data:
            return None

        data = super(Resp, cls).de_json(data, client)
        return cls(client=client, **data)
