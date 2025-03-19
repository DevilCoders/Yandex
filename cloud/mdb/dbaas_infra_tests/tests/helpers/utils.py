"""
General purpose stuff, like dict merging or str.format() template filler.
"""
import collections.abc
import logging
import re
from functools import wraps
from importlib import import_module

from jinja2 import Template


def merge(original, update):
    """
    Recursively merge update dict into original.
    """
    for key in update:
        recurse_conditions = [
            # Does update have same key?
            key in original,
            # Do both the update and original have dicts at this key?
            isinstance(original.get(key), dict),
            isinstance(update.get(key), collections.abc.Mapping),
        ]
        if all(recurse_conditions):
            merge(original[key], update[key])
        else:
            original[key] = update[key]
    return original


def format_object(obj, **replacements):
    """
    Replace format placeholders with actual values
    """
    if isinstance(obj, str):
        obj = obj.format(**replacements)
    elif isinstance(obj, collections.abc.Mapping):
        for key, value in obj.items():
            obj[key] = format_object(value, **replacements)
    elif isinstance(obj, collections.abc.Iterable):
        for idx, val in enumerate(obj):
            obj[idx] = format_object(val, **replacements)
    return obj


def split(string, separator=',', strip=True):
    """
    Split string on tokens using the separator, optionally strip them and
    return as list.
    """
    return [v.strip() if strip else v for v in string.split(separator)]


def env_stage(event, fail=False):
    """
    Nicely logs env stage
    """

    def decorator(fun):
        @wraps(fun)
        def wrapper(*args, **kwargs):
            stage_name = '{mod}.{fun}'.format(
                mod=fun.__module__,
                fun=fun.__name__,
            )
            logging.info('initiating %s stage %s', event, stage_name)
            try:
                return fun(*args, **kwargs)
            except Exception as exc:
                logging.error('%s failed: %s', stage_name, exc)
                if fail:
                    raise

        return wrapper

    return decorator


def importobj(objpath):
    """
    Import object by path
    """
    module_path, obj_name = objpath.rsplit('.', 1)
    module = import_module(module_path)
    return getattr(module, obj_name)


def context_to_dict(context):
    """
    Convert behave context to dict representation.
    """
    result = {}
    for frame in context._stack:  # pylint: disable=protected-access
        for key, value in frame.items():
            if key not in result:
                result[key] = value

    return result


def render_template(template, template_context):
    """
    Render jinja template
    """
    return Template(template).render(**template_context)


def camelcase_to_underscore(obj):
    """
    Converts camelcase to underscore
    """
    return re.compile(r'[A-Z]').sub(lambda m: '_' + m.group().lower(), obj)


def convert_prefixed_upperscore(obj):
    """
    Converts `WAL_LEVEL_LOGICAL` to `logical`
    """
    if not isinstance(obj, str):
        return obj
    reg = re.compile(r'[A-Z_]+')
    if reg.match(obj):
        return reg.sub(lambda m: m.group().split('_')[-1].lower(), obj)
    return obj


def dict_convert_keys(obj):
    """
    Recursively converts keys of `obj` from camelcase to underscore
    """
    new_obj = dict()
    for key, value in obj.items():
        if isinstance(value, dict):
            dict_convert_keys(value)
        else:
            # Convert key to underscore, value 'WAL_LEVEL_LOGICAL' to 'logical'
            new_obj[camelcase_to_underscore(key)] = convert_prefixed_upperscore(value)
    return new_obj
