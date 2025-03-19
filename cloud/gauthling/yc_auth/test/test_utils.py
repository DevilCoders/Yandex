import pytest

from yc_auth.exceptions import AuthFailureError
from yc_auth.utils import decode_token, _restore_padding


def test_decode_token(encoded_token):
    token_bytes = encoded_token
    token_string = token_bytes.decode()
    assert decode_token(token_bytes)
    assert decode_token(token_string)

    invalid_base64_sequence = b"a"
    with pytest.raises(AuthFailureError):
        decode_token(invalid_base64_sequence)

    num_value = 42
    with pytest.raises(AuthFailureError):
        decode_token(num_value)


def test_restore_padding():
    data = b"YQ=="
    data_stripped = data.rstrip(b'=')
    data_restored = _restore_padding(data_stripped)
    assert data_restored == data

    data_no_padding = b"YWJj"
    data_no_padding_restored = _restore_padding(data_no_padding)
    assert data_no_padding_restored == data_no_padding
