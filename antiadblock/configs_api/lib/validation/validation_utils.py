# coding=utf-8
import sys
import traceback

import flask
from voluptuous import AllInvalid, Schema, Invalid, MultipleInvalid, Match, Length  # noqa

from antiadblock.configs_api.lib.db import Config


class Every(object):
    """
    Like voluptus.All but all validators test data even if one of them fails. Validators output is ignored.
    :returns the original value

    :param msg: Message to deliver to user if validation fails.
    :param kwargs: All other keyword arguments are passed to the sub-Schema constructors.

    >>> validate = Schema(Every(Length(max=20), Match(r'[^<^>^\\{^\\}]')))
    >>> validate('not html string')
    'not html string'
    """

    def __init__(self, *validators, **kwargs):
        self.validators = validators
        self.msg = kwargs.pop('msg', None)
        self._schemas = [Schema(val, **kwargs) for val in validators]

    def __call__(self, v):
        errors = []

        for schema in self._schemas:
            try:
                schema(v)
            except MultipleInvalid as e:
                errors += e.errors
            except Invalid as e:
                error = e if self.msg is None else AllInvalid(self.msg)
                errors.append(error)
        if errors:
            raise MultipleInvalid(errors)
        return v

    def __repr__(self):
        return 'Every(%s, msg=%r)' % (
            ", ".join(repr(v) for v in self.validators),
            self.msg)


class Ordered(object):
    """
    Checks that elements are ordered as specified by param order
    :returns the original value

    :param msg: Message to deliver to user if validation fails.

    >>> validate = Schema(Ordered([1, 2, 3]))
    >>> validate([1, 2, 3])
    [1, 2, 3]

    >>> validate([2, 1, 3])
    Traceback (most recent call last):
    ...
    MultipleInvalid: should be in order [1, 2, 3]

    >>> validate([1, 2, 3, 4])
    Traceback (most recent call last):
    ...
    MultipleInvalid: element is not present in order specification
    """

    def __init__(self, order, msg=None):
        self.msg = msg
        self.order = order

    def __call__(self, l):
        if not isinstance(l, (list, tuple)):
            raise Invalid("should be list or tuple")
        previous = 0
        for value in l:
            if value not in self.order:
                raise Invalid("element is not present in order specification")
            i = self.order.index(value)
            if previous > i:
                msg = self.msg or "should be in order {}".format(self.order)
                raise Invalid(msg)
            previous = i
        return l

    def __repr__(self):
        return 'Sorted(%s, msg=%r)' % (
            ", ".join(repr(v) for v in self.validators),
            self.msg)


def prepend_invalid_exception_path(prepend_path):
    def prepend(f):
        def adapter(*args, **kvargs):
            try:
                return f(*args, **kvargs)
            except Invalid as ex:
                ex.prepend([prepend_path])
                raise ex
        return adapter
    return prepend


def exception_to_invalid(f, msg=None):
    """
    If validation doesn't work do not allow to create config
    :param msg:
    :param f:
    :return:
    """
    def catch(*args, **kvargs):
        try:
            return f(*args, **kvargs)
        except Invalid:
            raise
        except Exception as ex:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            flask.current_app.logger.error("{0.__class__.__name__} : {0}".format(ex),
                                           dict(exception="".join(traceback.format_exception(exc_type, exc_value, exc_traceback, limit=3))))
            raise Invalid(message=msg or u"Настройки не могут быть прочитаны", path=[Config.data.name])
    return catch


def is_schema_ok(schema, data):
    """returns True if schema check is ok"""
    try:
        schema(data)
    except Invalid:
        return False
    return True


def return_validation_errors(prefix_path):
    """
    prepend Invalid exception path with prefix.
    :param f:
    :return: [] if everything is alright
    """
    def dec(f):
        def prepend(*args, **kvargs):
            try:
                f(*args, **kvargs)
                return []
            except MultipleInvalid as ex:
                ex.prepend(prefix_path)
                return ex.errors
            except Invalid as ex:
                ex.prepend(prefix_path)
                return [ex]
        return prepend
    return dec
