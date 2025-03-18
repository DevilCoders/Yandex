# -*- coding: utf-8 -*-

import time
from collections import defaultdict
from copy import copy


class LocalMemoryStorage(object):
    '''
    Класс хранилища.
    Простое хранилище в памяти.
    Поддерживается хранение записей заданное количество времени.
    '''

    def __init__(self, timeout=None):
        """
        @param timeout: таймаут в секундах
        """
        super(LocalMemoryStorage, self).__init__()
        self.resources = defaultdict(lambda: None)
        self._expire_info = {}
        self._timeout = timeout

    def add(self, key, obj):
        exp = self._expire_info.get(key)
        if exp is None or exp <= time.time():
            self.resources[key] = obj
            if self._timeout:
                self._expire_info[key] = time.time() + self._timeout
            return True
        return False

    def set(self, key, obj):
        self.resources[key] = obj
        if self._timeout:
            self._expire_info[key] = time.time() + self._timeout

    def get(self, key):
        exp = self._expire_info.get(key)
        if exp is None or exp > time.time():
            return copy(self.resources[key])
        else:
            self.delete(key)
            return None

    def delete(self, key):
        if self.resources[key] is None:
            return False
        del self.resources[key]
        if key in self._expire_info:
            del self._expire_info[key]
        return True
