# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import json

from ids.registry import registry

from .base import UatraitsRepository


@registry.add_simple
class UatraitsDetectRepository(UatraitsRepository):
    SERVICE = 'uatraits'
    RESOURCES = 'detect'

    def get_one(self, lookup, **options):
        if 'data' in lookup:
            data = lookup.get('data')
        else:
            data = json.dumps({'User-Agent': lookup.get('user_agent', ' ')})

        return self.post(lookup, data=data, **options)
