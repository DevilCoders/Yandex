# coding: utf-8

from abc import ABCMeta, abstractmethod


class Logger(object):
    __metaclass__ = ABCMeta

    @abstractmethod
    def log_context(self, **kwargs):
        pass

    @abstractmethod
    def info(self, *args, **kwargs):
        pass
