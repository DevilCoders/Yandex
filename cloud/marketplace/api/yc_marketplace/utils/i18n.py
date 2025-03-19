import functools

from cloud.marketplace.common.yc_marketplace_common.lib.i18n import I18n

# TODO move to api_handler


def i18n_traverse(to_sort=None):
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            return I18n.traverse_simple(func(*args, **kwargs), to_sort=to_sort)
        return wrapper
    return decorator
