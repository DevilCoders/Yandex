# coding: utf-8

from __future__ import division, absolute_import, print_function, unicode_literals
import sys
import os
import re
import json
import pkgutil
import six
from six.moves import range


PY_3 = sys.version_info >= (3,)


class NestedDict(dict):
    """Hierarchical dict with frozen keys.

    Leaf-values are mutable, all keys are immutable.
    """
    def _blocked_attribute(
            self,
            *args,
            **kwargs):  # pylint: disable=no-self-use
        raise AttributeError('structure cannot be modified.')

    __delitem__ = clear = pop = popitem = setdefault = _blocked_attribute

    def __init__(self, *args, **kwargs):
        super(NestedDict, self).__init__(*args, **kwargs)
        for key, value in six.iteritems(self):
            if isinstance(value, dict):
                self.__base_setitem__(key, NestedDict(value))
            elif isinstance(value, six.binary_type):
                self.__base_setitem__(key, six.text_type(value))

    def __base_setitem__(self, *args, **kwargs):
        return super(NestedDict, self).__setitem__(*args, **kwargs)

    def update(self, new_content):
        for key, new_value in six.iteritems(new_content):
            if key not in self:
                raise KeyError(key)
            if isinstance(self[key], dict):
                if not isinstance(new_value, dict):
                    text = 'got {} instead {}'
                    raise ValueError(text.format(new_value, type(self[key])))
                self[key].update(new_value)
            else:
                if self[key] is not None and \
                        not isinstance(new_value, type(self[key])):
                    text = 'got {} instead {}'
                    raise ValueError(text.format(new_value, type(self[key])))
                self.__base_setitem__(key, new_value)

    def __setitem__(self, key, value):
        self.update({key: value})

    def to_dict(self):
        def flat(value):
            try:
                return value.to_dict()
            except AttributeError:
                return value
        return {key: flat(value) for key, value in six.iteritems(self)}

    def __repr__(self):
        return str(self.to_dict())


def pretty_dump(obj, indent=2):
    if isinstance(obj, six.string_types):
        return obj
    try:
        return json.dumps(obj, indent=indent, sort_keys=True)
    except Exception:  # pylint: disable=broad-except
        return str(obj)


def dump_raw_request(request, max_dump_len=2048, indent=2):
    headers = request.headers or {}
    auth_headers = set(('authorization', 'cookie', 'statrobotpassword'))
    headers = {
        key: (
            value[:3] + '...'  # e.g. 'OAu...'
            if six.text_type(key).lower() in auth_headers
            else value)
        for key, value in headers.items()}

    request_info = dict(
        url=request.url,
        method=request.method or 'get',
        headers=headers,
        params=request.params or {},
        data=request.data or {}
    )

    indent_str = ' ' * indent

    def dump_value(val):
        val = pretty_dump(val, indent=indent)
        val = val.replace('\n', '\n{}'.format(indent_str))
        val = re.sub(r' +\n', '\n', val)
        return val

    request_info = {
        key: dump_value(value)
        for key, value in request_info.items()}

    pattern = '\n'.join((
        u'{indent}request {method} url: {url}',
        u'{indent}headers: {headers}',
        u'{indent}params: {params}',
        u'{indent}data: {data}',
    ))
    message = pattern.format(indent=indent_str, **request_info)

    if len(message) > max_dump_len:
        message = message[:max_dump_len] + '... truncated message.'

    return message


def dump_response(response, headers=('X-Code-Version', 'X-Request-Id'), indent=2):
    try:
        error_pouch = dict(response.json())
        text = error_pouch.pop('long_message', None) or \
            error_pouch.pop('message', '')
        text += '\n' + pretty_dump(error_pouch)
    except Exception:  # pylint: disable=broad-except
        text = response.text
    indent_str = ' ' * indent
    template = '\n'.join((
        u'{indent}response status {resp.status_code}({resp.reason})',
        u'{indent}headers: {headers}',
        u'{indent}elapsed {resp.elapsed}',
        u'{text}',
    ))
    headers_data = [(key, response.headers.get(key)) for key in headers]
    headers_data = [(key, val) for key, val in headers_data if val is not None]
    headers_str = '; '.join(['{}: {}'.format(key, val) for key, val in headers_data] + ['...'])
    return template.format(resp=response, headers=headers_str, indent=indent_str, text=text)


def current_frame(depth=1):
    """
    Copypaste from `sbdutils.base`.
    (from logging/__init__.py)
    """
    func = getattr(sys, '_getframe', None)
    if func is not None:
        return func(depth)
    # fallback; probably not relevant anymore.
    try:
        raise Exception
    except Exception:   # pylint: disable=broad-except
        frame = sys.exc_info()[2].tb_frame
        for _ in range(depth):
            frame = frame.f_back


def find_caller(extra_depth=1, skip_packages=()):
    """
    Find the stack frame of the caller so that we can note the source
    file name, line number and function name.

    Copypaste from `sbdutils.base`. Mostly a copypaste from `logging`.

    :param skip_packages: ...; example: `[getattr(logging, '_srcfile', None)]`.
    """
    cur_frame = current_frame(depth=2 + extra_depth)  # our caller, i.e. parent frame
    frame = cur_frame
    # On some versions of IronPython, currentframe() returns None if
    # IronPython isn't run with -X:Frames.
    result = "(unknown file)", 0, "(unknown function)"
    while hasattr(frame, "f_code"):
        codeobj = frame.f_code
        filename = os.path.normcase(codeobj.co_filename)
        # Additionally skip
        if any(filename.startswith(pkg) for pkg in skip_packages if pkg):
            frame = frame.f_back
            continue
        result = (codeobj.co_filename, frame.f_lineno, codeobj.co_name)
        break
    return result


def is_arcadia():
    return pkgutil.find_loader('__res') is not None


def get_cacert_path():
    return os.path.realpath(
        os.path.join(
            os.path.dirname(__file__),
            'cacert.pem'
        )
    )


def _get_statinfra_job_context():
    try:
        # pylint: disable=no-name-in-module,import-error
        from statinfra.common.tools import get_job_id
        return dict(name=get_job_id())
    except Exception:  # pylint: disable=broad-except
        return None


def _get_statinfra_action_context():
    try:
        # pylint: disable=no-name-in-module,import-error
        from statinfra.action import get_action_id
        return dict(name=get_action_id())
    except Exception:  # pylint: disable=broad-except
        return None


def _get_nirvana_context():
    try:
        # pylint: disable=no-name-in-module,import-error
        import nirvana.job_context as nv
    except Exception:  # pylint: disable=broad-except
        return None
    try:
        ctx = nv.context()
        meta = ctx.get_meta()
        block_url = meta.get_block_url()
        return dict(
            url=block_url,
            # e.g. for user-agent. Any better ideas?
            name=block_url,
        )
    except Exception:  # pylint: disable=broad-except
        # TODO?: log?
        return None


def get_process_context():
    result = (
        _get_statinfra_job_context() or
        _get_statinfra_action_context() or
        _get_nirvana_context() or
        {}
    )
    ctx_url = os.environ.get('CONTEXT_URL')
    if ctx_url and not result.get('url'):
        result['url'] = ctx_url

    # Might set some 'current process correlation id' in here.
    result['x_request_id'] = None

    return result
