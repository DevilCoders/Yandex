import re
from typing import Mapping

from click import ClickException


class KeyException(ClickException):
    pass


def get_key(object, key_path, default=None, separators=[':', '.', ',']):
    """
    Get key value from object with arbitrary number of nested lists and dics.
    """

    def _get_key(obj, path):
        key = path.pop(0)

        if isinstance(obj, list):
            try:
                key = int(key)
            except Exception:
                raise default

        if not path:
            try:
                return obj[key]
            except Exception:
                return default
        else:
            try:
                return _get_key(obj[key], path)
            except Exception:
                return default

    return _get_key(object, _split_key_path(key_path, separators))


def update_key(object, key_path, value, separators=[':', '.', ',']):
    """
    Update key value in object with arbitrary number of nested lists and dics.
    """

    def _update_key(obj, path, value, current_path_str):
        key = path.pop(0)
        current_path_str = f'{current_path_str}."{key}"' if current_path_str else f'"{key}"'

        if isinstance(obj, list):
            try:
                key = int(key)
            except Exception:
                raise KeyException(f'Key path "{key_path}" is invalid as {current_path_str} is a list.')

        if not path:
            if isinstance(obj, list):
                list_size = len(obj)
                if key < list_size:
                    obj[key] = value
                elif key == list_size:
                    obj.append(value)
                else:
                    raise KeyException(
                        f'Key path "{key_path}" is invalid as {current_path_str} is a list with {list_size} elements.'
                    )
            else:
                obj[key] = value
        else:
            if isinstance(obj, list):
                list_size = len(obj)
                if key < len(obj):
                    _update_key(obj[key], path, value, current_path_str)
                else:
                    raise KeyException(
                        f'Key path "{key_path}" is invalid as {current_path_str} is a list with {list_size} elements.'
                    )
            else:
                if obj.get(key) is None:
                    obj[key] = {}
                _update_key(obj[key], path, value, current_path_str)

    _update_key(object, _split_key_path(key_path, separators), value, '')


def delete_key(object, key_path, separators=[':', '.', ',']):
    """
    Delete key from object with arbitrary number of nested lists and dics. Do nothing if the key was not found.
    """

    def _delete_key(obj, path):
        key = path.pop(0)

        if isinstance(obj, list):
            try:
                key = int(key)
            except Exception:
                return

        if not path:
            try:
                del obj[key]
            except Exception:
                return
        else:
            try:
                _delete_key(obj[key], path)
            except Exception:
                return

    _delete_key(object, _split_key_path(key_path, separators))


def expand_key_pattern(object, key_pattern, separators=[':', '.', ',']):
    def _expand(prefix, obj, path):
        # terminate condition
        if not path:
            yield prefix
            return

        # add separator
        if prefix:
            prefix = f'{prefix}{separators[0]}'

        # expand "*"
        key = path.pop(0)
        if key == '*':
            if isinstance(obj, list):
                for i, value in enumerate(obj):
                    yield from _expand(f'{prefix}{i}', value, path.copy())
            elif isinstance(obj, Mapping):
                for key, value in obj.items():
                    yield from _expand(f'{prefix}{key}', value, path.copy())
            return

        # expand ordinary key element
        if isinstance(obj, list):
            try:
                value = obj[int(key)]
            except Exception:
                value = {}
        elif isinstance(obj, Mapping):
            value = obj.get(key, {})
        else:
            value = {}

        yield from _expand(f'{prefix}{key}', value, path)

    return list(_expand('', object, _split_key_path(key_pattern, separators)))


def _split_key_path(key_path, separators):
    if isinstance(key_path, str):
        regexp = '[{separators}]'.format(separators=''.join(separators))
        return re.split(regexp, key_path)

    return list(key_path)


def to_overlay_fqdn(ctx, fqdn):
    """
    Transform host fqdn to overlay fqdn.
    """
    overlay_suffix, underlay_suffix = _host_suffixes(ctx)
    if not overlay_suffix or not underlay_suffix:
        return fqdn

    return fqdn.replace(underlay_suffix, overlay_suffix)


def to_overlay_fqdn_list(ctx, fqdn_list):
    if fqdn_list is None:
        return None

    return [to_overlay_fqdn(ctx, fqdn) for fqdn in fqdn_list]


def to_underlay_fqdn(ctx, fqdn):
    """
    Transform host fqdn to underlay fqdn.
    """
    overlay_suffix, underlay_suffix = _host_suffixes(ctx)
    if not overlay_suffix or not underlay_suffix:
        return fqdn

    return fqdn.replace(overlay_suffix, underlay_suffix)


def to_underlay_fqdn_list(ctx, fqdn_list):
    if fqdn_list is None:
        return None

    return [to_underlay_fqdn(ctx, fqdn) for fqdn in fqdn_list]


def _host_suffixes(ctx):
    config = ctx.obj['config']['intapi']
    return config.get('overlay_host_suffix', None), config.get('underlay_host_suffix', None)
