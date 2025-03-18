# coding: utf-8
from __future__ import unicode_literals

from copy import deepcopy


class CachedChainTaskMixin(object):
    """
    Миксин для celery.Task.
    Кэширует chain в request в момент запуска,
    чтобы в дальнейшем получить именно то состояние, которое было вначале.
    Нужен для корректной работы DatabaseExtendedBackend
    """
    def __call__(self, *args, **kwargs):
        chain = getattr(self.request, 'chain', []) or []
        self.request.cached_chain = deepcopy(chain)
        return super(CachedChainTaskMixin, self).__call__(*args, **kwargs)
