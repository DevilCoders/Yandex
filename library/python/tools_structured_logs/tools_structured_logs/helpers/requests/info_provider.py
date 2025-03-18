# coding: utf-8

import logging
from six.moves import urllib_parse as urlparse

from tools_structured_logs.logic.log_records.vendors.http.info_provider import HttpRequestProvider
from tools_structured_logs.logic.configuration.library import get_config


class RequestsLibDomainProvider(HttpRequestProvider):
    context_requires = [
        'request',
        'response',
    ]

    def get_url(self):
        return self.sanitize(self._context['request'].url)

    def get_http_method(self):
        return self._context['request'].method

    def get_status_code(self):
        return getattr(self._context['response'], 'status_code', None)

    def get_hostname(self):
        return self.parsed_url().hostname

    def get_query(self):
        return self.parsed_url().query

    def get_content(self):
        max_size = get_config().get_http_response_max_size()
        if max_size > 0:
            if self.get_status_code() not in (None, 200, 404):
                try:
                    content = getattr(self._context['response'], 'content', '')[:max_size]
                except Exception as exc:
                    logging.getLogger(__name__).exception('cannot get content for %s: "%s"', self.get_url(), exc)
                else:
                    return content
        return None

    def duration(self):
        return self._context['response'].elapsed.total_seconds() * 1000

    def parsed_url(self):
        if 'parsed_url' not in self._context:
            self._context['parsed_url'] = urlparse.urlparse(self.get_url())
        return self._context['parsed_url']

    def get_path(self):
        return self.parsed_url().path

    def vendor(self):
        return 'requests'
