# -*- coding: utf-8 -*-


class Resource(dict):
    '''
    Представляет объект, возвращаемый репозитоиями для отображения данных.
    '''

    def __init__(self, *args, **kwargs):
        super(Resource, self).__init__(*args, **kwargs)


def wrap(obj):
    return Resource(obj) if obj is not None else None
