# coding: utf-8

import memcache


class MemcachedStorage(object):
    '''
    Класс хранилища.
    Реализация хранилища в Memcache
    '''

    def __init__(self, servers=['127.0.0.1:11211']):
        '''
        @param servers: list
            Список адресов серверов memcached.
            По умолчанию ['127.0.0.1:11211']
        '''
        super(MemcachedStorage, self).__init__()
        self.memcache = memcache.Client(servers, debug=0)

    def add(self, key, obj):
        return self.memcache.add(key, obj)

    def set(self, key, obj):
        return self.memcache.set(key, obj)

    def get(self, key):
        return self.memcache.get(key)

    def delete(self, key):
        self.memcache.delete(key)
        return True  # memcached always return True
