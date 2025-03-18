import typing

from ..types import Headers


def get_real_ip(headers: Headers, config: 'AsgiYauthConfig') -> typing.Optional[str]:
    for header in config.ip_headers:
        if header in headers:
            return headers[header].split(',')[0]
