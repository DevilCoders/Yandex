# coding: utf-8

from __future__ import unicode_literals

import operator

from . import datetimes


class DateRangeable(list):

    date_field = None
    value_field = None
    value_wrapper = None
    getter = operator.itemgetter
    empty_values = {None}

    def __init__(self, iterable=None, **kwargs):
        iterable = iterable or []
        self.date_field = self.date_field or kwargs.pop('date_field')
        self.value_field = self.value_field or kwargs.pop('value_field', None)
        self.value_wrapper = self.value_wrapper or kwargs.pop('value_wrapper', None)
        self.getter = kwargs.pop('getter', self.getter)
        iterable = self._prepare_items(iterable)
        super(DateRangeable, self).__init__(iterable)

    def _prepare_items(self, items):
        """
        Выкидываем бездатовые события (например, когда от нас скрыли данные
        топов — там отдаются объекты с null во всех полях)
        """
        result = []
        for item in items:
            if not self.date_getter(item):
                continue
            result.append(item)

        result.sort(key=self.date_getter)
        return result

    @property
    def date_getter(self):
        return self.getter(self.date_field)

    @property
    def value_getter(self):
        if self.value_field is None:
            raise RuntimeError('Method requires `value_field` to be configured')
        return self.getter(self.value_field)

    def get_date_from_item(self, item):
        return datetimes.parse_date(self.date_getter(item))

    def wrap_value(self, value):
        if value is None:
            return value
        if self.value_wrapper is not None:
            return self.value_wrapper(value)
        return value

    def get_value_from_item(self, item):
        value = self.value_getter(item)
        value = self.wrap_value(value)
        return value

    def get_range(self, from_date=None, to_date=None):
        """
        Полуинтервал [from_date; to_date)
        """
        result = self.__class__(
            date_field=self.date_field,
            value_field=self.value_field,
            getter=self.getter,
        )
        from_date = from_date and datetimes.parse_date(from_date)
        to_date = to_date and datetimes.parse_date(to_date)
        for item in self:
            date = self.get_date_from_item(item)
            if from_date is not None and date < from_date:
                continue
            if to_date is not None and date >= to_date:
                continue
            result.append(item)
        return result

    def latest(self):
        if self:
            return self[-1]

    def earliest(self):
        if self:
            return self[0]

    def get_closest_before(self, date):
        date = datetimes.parse_date(date)
        prev_item = None
        for item in self:
            cur_date = self.get_date_from_item(item)
            if cur_date > date:
                return prev_item
            prev_item = item
        return prev_item

    def get_range_values(self, from_date=None, to_date=None, skip_empty=True):
        subrange = self.get_range(from_date, to_date)
        result = []
        for item in subrange:
            value = self.get_value_from_item(item)
            if value in self.empty_values and skip_empty:
                continue
            result.append(value)
        return result

    def get_range_values_reduced(
        self,
        from_date=None, to_date=None,
        skip_empty=True, reducer=None,
    ):
        return reducer(self.get_range_values(from_date, to_date, skip_empty))

    def get_range_values_sum(self, from_date=None, to_date=None):
        return self.get_range_values_reduced(from_date, to_date, reducer=sum)

    def get_range_values_max(self, from_date=None, to_date=None):
        return self.get_range_values_reduced(from_date, to_date, reducer=max)

    def get_range_values_min(self, from_date=None, to_date=None):
        return self.get_range_values_reduced(from_date, to_date, reducer=max)

    @classmethod
    def get_closest_before_date(cls, iterable, date):
        date_getter = cls.getter(cls.date_field)
        closest_date = None
        closest = None
        for item in iterable:
            item_date = datetimes.parse_date_isoformat(date_getter(item))
            if item_date > date:
                continue
            if closest_date is None or closest_date <= item_date:
                closest_date = item_date
                closest = item
        return closest
