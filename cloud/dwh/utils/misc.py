import urllib.parse
from typing import Optional


def drop_none(d: dict, none=None) -> dict:
    """Drops keys with none value. Not nested"""
    return {k: v for k, v in d.items() if v is not none}


def format_url(url: str, query_params: Optional[dict] = None) -> str:
    if not query_params:
        return url

    scheme, netloc, path, url_params, url_query, fragment = urllib.parse.urlparse(url)

    query_params_str = urllib.parse.urlencode(query_params, doseq=True)
    if url_query:
        url_query = '&'.join((url_query, query_params_str))
    else:
        url_query = query_params_str

    return urllib.parse.urlunparse((scheme, netloc, path, url_params, url_query, fragment))


def ellipsis_string(s: str, max_length: int, ellipsis_: str = '>...<') -> str:
    if len(s) <= max_length:
        return s

    length = (max_length - len(ellipsis_)) // 2
    if length < 1:
        return ellipsis_

    return s[:length] + ellipsis_ + s[-length:]
