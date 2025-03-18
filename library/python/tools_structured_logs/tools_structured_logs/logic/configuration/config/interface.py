# coding: utf-8

import os

from abc import ABCMeta, abstractmethod


class Config(object):
    __metaclass__ = ABCMeta

    def get_requests_modules_to_patch(self):
        return [
            'requests.sessions',
            'yt.packages.requests.sessions',
        ]

    @abstractmethod
    def get_context_providers(self):
        pass

    @abstractmethod
    def get_enable_tracking(self):
        return os.environ.get('YLOG_TRACKING', '1').lower() in ('1', 'true', 'yes')

    @abstractmethod
    def get_enable_db_tracking(self):
        """
        default True
        :return:
        """

    @abstractmethod
    def get_enable_http_tracking(self):
        """
        default True
        :return:
        """

    @abstractmethod
    def get_enable_stack_traces(self):
        """
        default False
        :return:
        """

    @abstractmethod
    def get_http_response_max_size(self):
        """
        character length, default 0
        :return:
        """

    @abstractmethod
    def get_sql_warning_threshold(self):
        """
        msec, default 500
        :return:
        """
