# coding: utf-8

from abc import ABCMeta, abstractmethod
import uuid

from .base import BaseProvider


def _get_request_id(request):
    return request.META.get(
        'HTTP_X_REQ_ID',
        request.META.get(
            'HTTP_X_REQUEST_ID',
            request.GET.get(
                'request_id',
            )
        )
    ) or None


class Provider(BaseProvider):
    __metaclass__ = ABCMeta
    required_kwargs = ['request']

    @property
    def request_id_not_provided(self):
        return 'auto-' + uuid.uuid4().hex

    def always_have_some_request_id(self, **ctx):
        r_id = self.get_request_id(**ctx)
        if r_id is None:
            r_id = self.request_id_not_provided
        return r_id

    @abstractmethod
    def get_request_id(self, **ctx):
        pass

    @abstractmethod
    def get_http_method(self, **ctx):
        pass

    @abstractmethod
    def get_request_path(self, **ctx):
        pass

    @abstractmethod
    def get_query_params(self, **ctx):
        pass

    def field_request(self, **ctx):
        return {
            'id': self.always_have_some_request_id(**ctx),
            'method': self.get_http_method(**ctx),
            'path': self.get_request_path(**ctx),
            'query_params': self.get_query_params(**ctx),
        }
