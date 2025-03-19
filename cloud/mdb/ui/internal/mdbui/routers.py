from typing import Optional
from django.db import models


class DBRouter(object):
    _readable_routes = {
        'deploy': 'deploy_slave',
        'meta': 'meta_slave',
        'dbm': 'dbm_slave',
        'katan': 'katan_slave',
        'cms': 'cms_slave',
    }
    _writable_routes = {
        'cms': 'cms_primary',
    }

    def db_for_read(self, model: models.Model, **_: dict) -> Optional[str]:
        if model._meta.app_label in self._readable_routes:
            return self._readable_routes[model._meta.app_label]
        return None

    def db_for_write(self, model: models.Model, **_: dict) -> Optional[str]:
        if model._meta.app_label in self._readable_routes:
            return self._writable_routes[model._meta.app_label]
        return None

    def allow_relation(self, obj1: models.Model, obj2: models.Model, **_: dict) -> bool:
        db_list = ('deploy_slave', 'meta_slave', 'dbm_slave', 'katan_slave', 'cms_slave', 'default')
        if obj1._state.db in db_list and obj2._state.db in db_list:
            return True
        return False
