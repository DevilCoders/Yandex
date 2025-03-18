# coding: utf-8
from __future__ import unicode_literals

import re

from django_pgaas.compat import DatabaseOperations as BaseDatabaseOperations
from django_pgaas.conf import settings

UPPER_REGEX = re.compile(r'^UPPER\((.+)\)$')


class DatabaseOperations(BaseDatabaseOperations):
    compiler_module = 'django_pgaas.backend.compiler'

    def lookup_cast(self, *args, **kwargs):
        cast = super(DatabaseOperations, self).lookup_cast(*args, **kwargs)
        return UPPER_REGEX.sub(r'UPPER(\1 {})'.format(settings.PGAAS_COLLATION), cast)
