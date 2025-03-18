# coding: utf-8

from __future__ import unicode_literals
from copy import deepcopy


class Page(object):
    """
    Страница выдачи пагинированного API.
    Класс обеспечивает доступ к полям страницы.

    Обязательные публичные методы
        * __iter__() — список объектов на странице

    Рекомендуемые методы и атрибуты
        * __len__() — возвращает число объектов на странице
        * total — общее число объектов
        * limit — максимальное число объектов на странице

        * page — номер текущей страницы (если известен)
        * pages — общее число страниц (вычисляется, если есть total и limit)
    """

    def __init__(self, data):
        self.set_page_attributes(data)

    def __iter__(self):
        raise NotImplementedError

    def set_page_attributes(self, data):
        for key, value in data.items():
            setattr(self, key, value)

    def __repr__(self):
        return '<{cls}: {identity}>'.format(
            cls=self.__class__.__name__,
            identity=' '.join(map(str, [
                getattr(self, 'page', '?'),
                ' of ',
                getattr(self, 'pages', '?'),
            ])),
        )

    # optional helpers
    @property
    def is_first(self):
        if not hasattr(self, 'page'):
            msg = 'is_first depends on `page` attribute'
            raise AttributeError(msg)
        return self.page == 1

    @property
    def is_last(self):
        if not hasattr(self, 'page') or not hasattr(self, 'pages'):
            msg = 'is_last depends on `page` and `pages` attributes'
            raise AttributeError(msg)
        return self.page == self.pages

    @property
    def pages(self):
        if not hasattr(self, 'total') or not hasattr(self, 'limit'):
            msg = 'pages depends on `total` and `limit` attributes'
            raise AttributeError(msg)
        import math
        return int(math.ceil(float(self.total) / self.limit))


class ResultSet(object):
    """
    Итерабельный объект, можно итерироваться прямо по нему и получить ВСЕ
    объекты репозитория, либо можно взять get_pages и итерироваться по каждой
    странице отдельно.

    Инициализируется объектом `fetcher`, который хранит параметры
    запроса и умеет получать страницу апи по номеру.

    Про страницу ResultSet знает только то, что она итерабельная.
    Про fetcher ResultSet знает, что у него есть атрибут first_page и методы
    has_page_after(current=<page>) и get_page(after=<page>).
    """

    def __init__(self, fetcher):
        self.fetcher = fetcher

    def __iter__(self):
        for page in self.get_pages():
            for obj in page:
                yield obj

    def get_pages(self):
        yield self.first_page
        current_page = self.first_page
        while self.fetcher.has_page_after(current=current_page):
            next_page = self.fetcher.get_page(after=current_page)
            yield next_page
            current_page = next_page

    @property
    def first_page(self):
        return self.fetcher.first_page

    @property
    def total(self):
        return self.first_page.total

    @property
    def pages(self):
        return self.first_page.pages


class Fetcher(object):
    """
    Объект, который умеет получить из API текущую страницу и страницу
    следующую за текущей.
    """

    page_cls = Page
    _first_page = None

    def __init__(self, connector, **request_params):
        self.connector = connector
        self.request_params = request_params

    @property
    def first_page(self):
        """
        Первая в смысле первая в выборке. Если пользователь явно укажет
        _page=100500, то в этом атрибуте будет эта страница.
        """
        if self._first_page is None:
            self._first_page = self.get_page()
        return self._first_page

    def get_page(self, after=None):
        if after is not None:
            params = self.get_request_params_for_page_after(current=after)
        else:
            params = self.request_params
        params = deepcopy(params)
        return self.page_cls(
            self.connector.get(**params)
        )

    def has_page_after(self, current):
        raise NotImplementedError

    def get_request_params_for_page_after(self, current):
        raise NotImplementedError
