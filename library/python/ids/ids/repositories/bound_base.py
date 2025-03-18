# -*- coding: utf-8 -*-

from ids.repositories.base import RepositoryBase


class BoundRepositoryBase(RepositoryBase):
    '''
    Ограниченный провайдер ресурсов.
    Доставляет ресурсы определенного типа, в ограниченном контексте.
    Например: доставляет комментарии, но привязанные только к одному тикету.
    '''

    OPTIONS_KEYS_TO_COPY = ()

    def __init__(self, parent_resource, storage=None, **options):
        super(BoundRepositoryBase, self).__init__(storage=storage, **options)
        self.parent_resource = parent_resource

        opts = self.parent_resource['__repository__'].options
        for key in self.OPTIONS_KEYS_TO_COPY:
            if key in opts:
                self.options[key] = opts[key]

    def _wrap_to_resource(self, obj, resource=None):
        r = super(BoundRepositoryBase, self)._wrap_to_resource(obj, resource)
        r['__parent__'] = self.parent_resource
        return r

    def get_user_session_id(self):
        return self.parent_resource['__repository__'].get_user_session_id()
