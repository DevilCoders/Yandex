# coding: utf-8

from abc import ABCMeta, abstractmethod
from .base import BaseProvider


class Provider(BaseProvider):
    __metaclass__ = ABCMeta

    @abstractmethod
    def get_org_id(self, **ctx):
        """
        id в Директории
        """

    def field_org(self, **ctx):
        return {
            'org_id': self.get_org_id(**ctx),
        }
