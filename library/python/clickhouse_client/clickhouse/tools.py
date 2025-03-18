# coding: utf8
"""

Note about quoting: clickhouse requests are generally done through urlencode,
which supports arbitrary binary data as-is, so using text_type for queries
anywhere would be limiting.
Do re-check this logic please, though.
"""

import contextlib
import logging
import re
from six import text_type, binary_type, string_types

import requests


logger = logging.getLogger('clickhouse_client')

NULL = b'\\N'


def u_repr(value, encoding="utf-8"):
    if isinstance(value, text_type):
        return value
    if isinstance(value, binary_type):
        return value.decode(encoding, errors="replace")
    return value


def sanitize_header(item):
    key, value = item
    if key.lower() in ('authorization', 'x-clickhouse-key'):
        value = '***'
    return key, value


def requests_error_to_details(exc):
        pattern = (
            u"Request {req.method} {req.url} failed with error:\n"
            u"{exc}\n"
            u"request body: {body}\n"
            u"request headers: {headers}"
        )
        details = ""

        request = getattr(exc, 'request', None)

        if request is not None:
            headers = dict(map(sanitize_header, request.headers.items()))
            details = pattern.format(exc=exc, req=request,
                                     body=u_repr(request.body),
                                     headers=headers)

        response = getattr(exc, 'response', None)
        if response is not None:
            pattern = (
                u"response status {resp.status_code}({resp.reason})\n"
                u"{resp.text}"
            )
            response_status = pattern.format(resp=response)
            # In case the password was in the params (not recommended, not default anymore), sanitize it.
            # py3 note: http://bugs.python.org/issue15096
            response_status = re.sub(u"password=[^&]+", u"password=***", response_status)
            details = u"{}\n{}".format(details, response_status)
        return details


@contextlib.contextmanager
def dump_request_error():
    try:
        yield
    except requests.exceptions.RequestException as exc:
        details = requests_error_to_details(exc)
        if details:
            logger.error(details)
        raise


def quote_identifier(name, force=False):
    assert isinstance(name, string_types)
    if isinstance(name, text_type):
        name = name.encode("utf-8")
    if not force and re.search(br"^[a-zA-Z_][0-9a-zA-Z_]*$", name):
        return name
    if b"`" not in name:
        return b"`" + name + "`"
    # Identifiers with "`" are, apparently, not valid.
    raise ValueError("Invalid identifier: %r" % (name,))


def quote_value(value, **kwargs):
    if isinstance(value, text_type):
        value = value.encode("utf-8")
    if isinstance(value, binary_type):
        # https://clickhouse.yandex/reference_en.html#String%20literals
        escaped = re.sub(br"([\\'])", br"\\\1", value)  # backslash-escape the backslash and single-quote
        return b"'" + escaped + "'"
    if isinstance(value, (int, float)):
        return binary_type(value)  # "close enough"
    if isinstance(value, list):
        # https://clickhouse.yandex/reference_en.html#Array%20creation%20operator
        assert len(value) >= 1, "clickhouse's requirement"
        children = (quote_value(child, **kwargs) for child in value)
        return b"[" + b", ".join(child for child in children) + b"]"
    if isinstance(value, tuple):
        # https://clickhouse.yandex/reference_en.html#Tuple%20creation%20operator
        assert len(value) >= 2, "clickhouse's requirement"
        children = (quote_value(child, **kwargs) for child in value)
        return b"(" + b", ".join(child for child in children) + b")"
    raise Exception("Unexpected value type")


def quote_parameters(parameters):
    if isinstance(parameters, dict):
        return {key: quote_value(value) for key, value in parameters.items()}
    return tuple(quote_value(value) for value in parameters)


class QuotedParameters(object):
    """ A slow but reliable version of `quote_parameters` (example) """

    def __init__(self, parameters, quote_value=quote_value):  # pylint: disable=redefined-outer-name
        self.parameters = parameters
        self.quote_value = quote_value

    def __getitem__(self, idx):
        return self.quote_value(self.parameters[idx])

    def __getattr__(self, name):
        return getattr(self.parameters, name)


def tsv_dump_value(value):
    if isinstance(value, text_type):
        value = value.encode("utf-8")
    value = binary_type(value)
    value = re.sub(br"([\\\n\t])", br"\\\1", value)
    return value


def tsv_dump(data):
    """
    https://clickhouse.yandex/reference_en.html#TabSeparated
    """
    ts_data = b"".join(
        b"\t".join(tsv_dump_value(value) if value is not None else NULL for value in row) + b"\n"
        for row in data)
    return ts_data


# ### ... ###
# https://github.com/Infinidat/infi.clickhouse_orm/blob/685e3dffe96e648459a6f5a504388de9e929859b/src/infi/clickhouse_orm/utils.py#L20

SPECIAL_CHARS = {
    b"\b" : b"\\b",
    b"\f" : b"\\f",
    b"\r" : b"\\r",
    b"\n" : b"\\n",
    b"\t" : b"\\t",
    b"\0" : b"\\0",
    b"\\" : b"\\\\",
    b"'"  : b"\\'"
}


SPECIAL_CHARS_REGEX = re.compile(b"[" + b"".join(SPECIAL_CHARS.values()) + b"]")


def quote_value_v2(value, quote=True):
    '''
    If the value is a string, escapes any special characters and optionally
    surrounds it with single quotes. If the value is not a string (e.g. a number),
    converts it to one.
    '''
    if isinstance(value, text_type):
        value = value.encode("utf-8")
    if isinstance(value, binary_type):
        if SPECIAL_CHARS_REGEX.search(value):
            value = b"".join(SPECIAL_CHARS.get(char, char) for char in value)
        if quote:
            value = b"'" + value + b"'"
    return binary_type(value)


def to_bytes(value, encoding='ascii', **kwargs):
    if isinstance(value, bytes):
        return value
    return value.encode(encoding, **kwargs)
