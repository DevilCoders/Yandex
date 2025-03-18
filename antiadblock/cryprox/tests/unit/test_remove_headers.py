import pytest
from antiadblock.cryprox.cryprox.common.tools.misc import remove_headers


@pytest.mark.parametrize('headers_to_remove', [['Host', 'Proxy-Connection'],  # production case
                                               []])  # no headers to remove
def test_remove_headers(headers_to_remove):
    not_remove_header = ['not_remove_header']
    headers = dict((key, 'value_for_{}'.format(key)) for key in (headers_to_remove + not_remove_header))
    headers_to_remove_prepared = [header.lower() for header in headers_to_remove]
    remove_headers(headers, headers_to_remove_prepared)
    assert headers.keys() == not_remove_header
