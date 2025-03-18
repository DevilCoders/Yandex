# -*- coding: utf-8 -*-


class NullStorage(object):
    '''
    Класс хранилища.
    Заглушка для случая отсутствия хранилища: пустой интерфейс.
    '''

    def __init__(self):
        super(NullStorage, self).__init__()

    def add(self, key, obj):
        '''
        @param key: object with __hash__()
        @param obj: object

        Добавляет объект в хранилище, если по ключу key в хранилище ничего нет.
        '''
        return False

    def set(self, key, obj):
        '''
        @param key: object with __hash__()
        @param obj: object

        Добавляет объект в хранилище.
        '''
        pass

    def get(self, key):
        '''
        @param key: object with __hash__()
        @returns: object or None

        Вовращает один объект из хранилища, доступный по ключу key.
        '''
        return None

    def delete(self, key):
        '''
        @param key: object with __hash__()
        @returns: bool

        Удаляет один объект из хранилища, доступный по ключу key.
        Возвращает признак удаления объекта.
        '''
        return False
