# coding: utf-8

import re
from abc import ABCMeta, abstractmethod

from tools_structured_logs.logic.configuration.library import get_config
from tools_structured_logs.logic.log_records.vendors.base_info_provider import IVendorInfoProvider
from .recorder import HttpRecorder


sessionid_re = re.compile(r'(sessionid|ts_sign)=([^&]+)', re.IGNORECASE)


class HttpRequestProvider(IVendorInfoProvider):
    __metaclass__ = ABCMeta

    @abstractmethod
    def get_http_method(self):
        pass

    @abstractmethod
    def get_status_code(self):
        pass

    @abstractmethod
    def get_hostname(self):
        pass

    @abstractmethod
    def get_path(self):
        pass

    @abstractmethod
    def get_query(self):
        pass

    @abstractmethod
    def get_content(self):
        pass

    @abstractmethod
    def get_url(self):
        pass

    def main_readable_string(self):
        return '{method} {url}'.format(
            method=self.get_http_method(),
            url=self.sanitize(self.get_url())
        )

    def query_to_analyse(self):
        return '{method} {short_url}'.format(
            method=self.get_http_method(),
            short_url=self.sanitize(self.parsed_url()._replace(query='', path='').geturl()),
        )

    def sanitize(self, url):
        return sessionid_re.sub(r'\1=xxxxx', url)

    def supported_recorders(self):
        return [
            HttpRecorder,
        ]

    def is_enabled(self):
        return get_config().get_enable_http_tracking()
