# coding: utf-8

import uuid

from .base import BaseProvider


class Provider(BaseProvider):
    def field_profiling_uuid(self, **ctx):
        return uuid.uuid1().hex
