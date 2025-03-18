"""
VARIOUS UTILS FOR SOME DIRTY WORK
"""

from monotonic import monotonic
from copy import deepcopy
from collections import namedtuple

import re2


REPLACE_MAP = {'&#x27;': '\'', '&amp;': '&'}
FIX_CSP_RE = re2.compile(r'(?:&#x27;|&amp;)')
CSP_HEADER_KEYS = ('content-security-policy', 'content-security-policy-report-only')

NONCE_RE = re2.compile(r'.*?(script|style|default)-src[^;]*\'nonce-(?P<nonce>[^.\']*)\'', re2.IGNORECASE)
HTML_META_CSP_CONTEXT_RE = re2.compile(r'<meta\s+http-equiv=\"?Content-Security-Policy\"?\s+content=\"([^\">]+?)\"(?:/)?>', re2.IGNORECASE)


def update_dict(base, merge):
    """
    updates `base` by inserting only new keys of `merge`
    makes it recursively
    """
    result = deepcopy(base)
    for key, value in merge.iteritems():
        if key in result.keys():
            if isinstance(value, dict):
                result[key] = update_dict(result[key], value)
        else:
            result[key] = value
    return result


def check_key_in_dict(data, key):
    """
    >>> check_key_in_dict("invalid_data", "key")
    False
    >>> check_key_in_dict({}, "key")
    False
    >>> check_key_in_dict({"another_key": "some_data"}, "key")
    False
    >>> check_key_in_dict({"key": "some_data"}, "key")
    True
    """
    return isinstance(data, dict) and key in data


def duration_usec(func, *args, **kwargs):
    """
    Calculate function operation time
    :param func: func to calculate duration
    :param args: func args
    :param kwargs: func kwargs
    :return: func operation time in microseconds
    """
    start = monotonic()
    result = func(*args, **kwargs)
    end = monotonic()
    return result, int((end - start) * 10 ** 6)


def int_list_to_bitmask(list_of_int):
    bitmask = 0
    for element in list_of_int:
        bitmask = (1 << element) | bitmask
    return bitmask


def is_bit_enabled(number, bit_nmb):
    """
    Check that a particular bit in the number is set (=1)
    :param number: number to check
    :param bit_nmb: bit position
    :return: True if bit is set, else False
    """
    return bool(number & (1 << bit_nmb))


def remove_headers(headers, exclude_list):
    """
    Simple method to delete headers from dict `headers`
    :param headers: dict with headers (for example tornado request.headers)
    :param exclude_list: list with headers to exclude from headers dict
    """
    for key in headers.keys():
        if key.lower() in exclude_list:
            del headers[key]


def not_none(iterable):
    """
    Filters out None values
    :returns list
    """
    return [i for i in iterable if i is not None]


def parse_nonce_from_headers(headers):
    """
    Try find 'nonce-<base64-value>' in directives script-src and default-src in headers 'Content-Security-Policy'
    :param headers: dict with headers
    :return: two strings with nonce for js and css if find else '', ''
    """
    keys = ('content-security-policy', 'content-security-policy-report-only')
    csp_headers = [v for k, v in headers.items() if k.lower() in keys]
    if len(csp_headers) == 0:
        return '', ''
    else:
        csp = csp_headers[0]

    # In primarily search for in directive script-src, if not found then default-src
    m = dict(NONCE_RE.findall(csp))
    nonce_js = m.get('script', m.get('default', ""))
    nonce_css = m.get('style', m.get('default', ""))
    return nonce_js, nonce_css


def replace_escape_symbols_in_csp(csp_string):

    def replace(matchobj):
        return REPLACE_MAP.get(matchobj.group(0), matchobj.group(0))

    return FIX_CSP_RE.sub(replace, csp_string)


def add_second_domain_to_csp(headers, body, domain):
    """
    Add any domain to csp headers and meta tags to response headers and html text, need for two domain crypt
    :param headers: response headers
    :param body: response body
    :param domain: domain to add
    :return: body
    """
    def add_domain_to_csp_string(csp_string):
        csp_string = replace_escape_symbols_in_csp(csp_string)
        return ';'.join(
            [
                x + ' ' + domain
                if ' ' in x.lstrip() and x.lstrip().split(' ', 1)[0].endswith('-src') and x.lstrip().split(' ', 1)[1].strip() != "'none'"
                else x for x in csp_string.split(';')
            ]
        )

    keys = ('content-security-policy', 'content-security-policy-report-only')
    new_csp_headers = {k: add_domain_to_csp_string(v) for k, v in headers.items() if k.lower() in keys}
    headers.update(new_csp_headers)

    result_body = HTML_META_CSP_CONTEXT_RE.sub(lambda m: replace_matched_group(m, 1, add_domain_to_csp_string(m.group(1))), body)
    return result_body


def add_nonce_to_csp(headers, body, nonce):
    """
    Add nonce to csp headers (script-src) and meta tags to response headers and html text, need for crypt content in inline js
    :param headers: response headers
    :param body: response body
    :param nonce: nonce to add
    :return: body
    """
    def add_nonce_to_csp_string(csp_string):
        results = []
        csp_string = replace_escape_symbols_in_csp(csp_string)
        for csp_src in csp_string.split(';'):
            # 'name value' -> ('name', 'value')
            _csp = csp_src.lstrip().split(' ', 1)
            if len(_csp) == 2 and _csp[0] == 'script-src' and _csp[1].strip() != "'none'":
                results.append(csp_src + ' \'nonce-{}\''.format(nonce))
            else:
                results.append(csp_src)
        return ';'.join(results)

    new_csp_headers = {k: add_nonce_to_csp_string(v) for k, v in headers.items() if k.lower() in CSP_HEADER_KEYS}
    headers.update(new_csp_headers)
    result_body = HTML_META_CSP_CONTEXT_RE.sub(lambda m: replace_matched_group(m, 1, add_nonce_to_csp_string(m.group(1))), body)
    return result_body


def add_patch_to_csp(headers, body, patched_csp):
    """
    Add patch to csp headers and meta tags to response headers and html text
    :param headers: response headers
    :param body: response body
    :param patched_csp: dict with patch
    :return: body
    """
    def add_patch_to_csp_string(csp_string):
        src_directives = []
        other_directives = []
        default_src_value = ''
        not_patched_keys = set(patched_csp.keys())
        csp_string = replace_escape_symbols_in_csp(csp_string)
        for csp in csp_string.split(';'):
            # 'name value' -> ('name', 'value')
            _csp = csp.lstrip().split(' ', 1)
            if len(_csp) == 2 and _csp[0] in patched_csp:
                not_patched_keys.difference_update([_csp[0]])
                if _csp[1].strip() != "'none'":
                    src_directives.append(csp + ' {}'.format(' '.join(patched_csp[_csp[0]])))
            elif "-src" in _csp[0]:
                src_directives.append(csp)
            else:
                other_directives.append(csp)

            if _csp[0] == 'default-src':
                default_src_value = _csp[1].strip()

        is_default_src_exist = default_src_value != "" and default_src_value != "'none'"

        # add not exist csp
        for csp_key in not_patched_keys:
            if is_default_src_exist:
                value = csp_key + ' ' + default_src_value + ' {}'.format(' '.join(patched_csp[csp_key]))
            else:
                value = csp_key + ' {}'.format(' '.join(patched_csp[csp_key]))
            src_directives.append(value.encode('utf-8'))
        return ';'.join(src_directives + other_directives)

    new_csp_headers = {k: add_patch_to_csp_string(v) for k, v in headers.items() if k.lower() in CSP_HEADER_KEYS}
    headers.update(new_csp_headers)

    result_body = HTML_META_CSP_CONTEXT_RE.sub(lambda m: replace_matched_group(m, 1, add_patch_to_csp_string(m.group(1))), body)
    return result_body


def replace_matched_group(match, group, replace_text):
    """
    Replace text from one group in match
    :param match: full match object
    :param group: group number or name from full match
    :param replace_text: replacement text
    :return: full match string, with replaced text
    """
    string = match.group()
    return string[:match.start(group) - match.start()] + replace_text + string[match.end(group) - match.start():]


def get_cache_control_max_age(cache_control_header):
    """
    >>> get_cache_control_max_age('max-age=3600, public')
    3600
    >>> get_cache_control_max_age('public, max-age=7200')
    7200
    >>> get_cache_control_max_age('private, max-age=7200')
    0
    >>> get_cache_control_max_age('max-age=315360000')
    315360000
    >>> get_cache_control_max_age('private, no-cache, no-store, must-revalidate, max-age=0')
    0
    >>> get_cache_control_max_age('')
    inf
    """
    seconds = float('inf')
    for directive in cache_control_header.split(','):
        directive = directive.strip()
        if directive in ('private', 'no-store', 'no-cache'):
            return 0
        elif directive.startswith('max-age'):
            seconds = int(directive.split('=')[1].strip())
    return seconds


class ConfigName(namedtuple('ConfigName', ['service_id', 'status', 'device_type', 'exp_id'])):
    def __new__(cls, key):
        if "::" not in key:
            service_id, status, device_type, exp_id = key, 'None', 'None', 'None'
        else:
            service_id, status, device_type, exp_id = key.split("::")
        return super(ConfigName, cls).__new__(cls, service_id, status, device_type, exp_id)
