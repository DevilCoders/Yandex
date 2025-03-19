DB_MAP = {
    'auth': 'default',
    'main': 'default',
    'cms': 'cms_db',
    'meta': 'meta_db',
    'deploy': 'deploy_db',
    'katan': 'katan_db',
    'dbm': 'dbm_db',
    'mlock': 'mlock_db',
}


class DBRouter:
    def db_for_read(self, model, **hints):
        return DB_MAP[model._meta.app_label]

    def db_for_write(self, model, **hints):
        return DB_MAP[model._meta.app_label]

    def allow_relation(self, obj1, obj2, **hits):
        if obj1._state.db == obj2._state.db:
            return True
        else:
            return False
