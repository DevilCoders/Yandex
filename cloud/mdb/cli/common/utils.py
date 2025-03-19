"""
Utility functions.
"""
from collections import defaultdict
from deepdiff import DeepDiff

import dateutil.parser
from dateutil.tz import gettz


def ensure_no_unsupported_options(ctx, kwargs, unsupported_options, msg):
    for arg_name, option_name in unsupported_options.items():
        if kwargs.get(arg_name):
            ctx.fail(msg.format(option_name))


def ensure_at_least_one_option(ctx, args, msg):
    if isinstance(args, dict):
        args = args.values()

    for value in args:
        if value is not None:
            return

    ctx.fail(msg)


def parse_timestamp(ctx, value, timezone=None):
    timestamp = dateutil.parser.parse(value)
    if timestamp.tzinfo is None:
        if timezone is None:
            timezone = get_timezone(ctx)

        timestamp = timestamp.replace(tzinfo=timezone)

    return timestamp


def get_timezone(ctx):
    if 'timezone' not in ctx.obj:
        config = ctx.obj['config']
        ctx.obj['timezone'] = gettz(config.get('timezone', 'UTC'))

    return ctx.obj['timezone']


def diff_objects(value1, value2):
    """
    Calculate structural diff between 2 values.
    """
    ignore_type_in_groups = [(dict, defaultdict)]
    return DeepDiff(value1, value2, verbose_level=2, view='tree', ignore_type_in_groups=ignore_type_in_groups)


class Nullable:
    def __init__(self, value=None):
        self.value = value


def flatten_nullable(value):
    if value is None:
        return False, None

    if isinstance(value, Nullable):
        value = value.value

    return True, value


def is_not_null(value):
    if isinstance(value, Nullable):
        value = value.value

    return value is not None
