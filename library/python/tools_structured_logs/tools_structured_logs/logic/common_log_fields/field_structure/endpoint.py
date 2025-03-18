# coding: utf-8

from .base import BaseProvider


class Provider(BaseProvider):
    required_kwargs = ['endpoint']

    def get_name(self, endpoint, **ctx):
        if hasattr(endpoint, '__name__'):
            name = endpoint.__name__
        else:
            name = endpoint.__class__.__name__
        return name

    def field_endpoint(self, endpoint, **ctx):
        return {
            'name': self.get_name(endpoint, **ctx),
        }
