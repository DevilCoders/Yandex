import base64
import binascii

import six

from .exceptions import AuthFailureError, ErrorMessages


def decode_token(token):
    if not isinstance(token, (six.string_types, six.binary_type)):
        raise AuthFailureError(ErrorMessages.InvalidToken)

    token_bytes = token.encode("utf-8") if isinstance(token, six.text_type) else token
    try:
        decoded_token = base64.urlsafe_b64decode(_restore_padding(token_bytes))
    except (binascii.Error, TypeError):
        raise AuthFailureError(ErrorMessages.InvalidToken)
    return decoded_token


def _restore_padding(token):
    """Restore padding based on token size.

    :param token: token to restore padding on
    :returns: token with correct padding

    """
    # Re-inflate the padding
    mod_returned = len(token) % 4
    if mod_returned:
        missing_padding = 4 - mod_returned
        token += b"=" * missing_padding
    return token
