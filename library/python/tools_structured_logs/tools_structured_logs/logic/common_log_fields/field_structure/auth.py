# coding: utf-8

from abc import ABCMeta, abstractmethod

from .base import BaseProvider


class Provider(BaseProvider):
    __metaclass__ = ABCMeta

    @abstractmethod
    def get_uid(self, **ctx):
        pass

    @abstractmethod
    def get_email(self, **ctx):
        pass

    @abstractmethod
    def get_auth_mechanism(self, **ctx):
        pass

    @abstractmethod
    def get_application(self, **ctx):
        pass

    def field_user(self, **ctx):
        return {
            'uid': self.get_uid(**ctx),
            'email': self.get_email(**ctx),
        }

    def field_auth(self, **ctx):
        return {
            'mechanism': self.get_auth_mechanism(**ctx),
            'application': self.get_application(**ctx),
        }
