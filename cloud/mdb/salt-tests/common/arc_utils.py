# Some arcadia-related functions

import os
import unicodedata


def is_arcadia():
    """Return true if we are in some Arcadia tests or something"""
    return 'ARCADIA_SOURCE_ROOT' in os.environ


def raise_if_not_arcadia(e):
    """
    Raise error e in case we are not inside of Arcadia tests
    Useful for importing salt modules
    """
    if not is_arcadia():
        raise e


def to_bytes(s, encoding=None, errors="strict"):
    """
    Given bytes, bytearray, str, or unicode (python 2), return bytes (str for
    python 2)
    """
    if encoding is None:
        # Try utf-8 first, and fall back to detected encoding
        encoding = ("utf-8",)
    if not isinstance(encoding, (tuple, list)):
        encoding = (encoding,)

    if not encoding:
        raise ValueError("encoding cannot be empty")

    exc = None
    if isinstance(s, bytes):
        return s
    if isinstance(s, bytearray):
        return bytes(s)
    if isinstance(s, str):
        for enc in encoding:
            try:
                return s.encode(enc, errors)
            except UnicodeEncodeError as err:
                exc = err
                continue
        # The only way we get this far is if a UnicodeEncodeError was
        # raised, otherwise we would have already returned (or raised some
        # other exception).
        raise exc  # pylint: disable=raising-bad-type
    raise TypeError("expected str, bytes, or bytearray not {}".format(type(s)))


def to_str(s, encoding=None, errors="strict", normalize=False):
    """
    Given str, bytes, bytearray, or unicode (py2), return str
    """

    def _normalize(s):
        try:
            return unicodedata.normalize("NFC", s) if normalize else s
        except TypeError:
            return s

    if encoding is None:
        # Try utf-8 first, and fall back to detected encoding
        encoding = ("utf-8",)
    if not isinstance(encoding, (tuple, list)):
        encoding = (encoding,)

    if not encoding:
        raise ValueError("encoding cannot be empty")

    if isinstance(s, str):
        return _normalize(s)

    exc = None
    if isinstance(s, (bytes, bytearray)):
        for enc in encoding:
            try:
                return _normalize(s.decode(enc, errors))
            except UnicodeDecodeError as err:
                exc = err
                continue
        # The only way we get this far is if a UnicodeDecodeError was
        # raised, otherwise we would have already returned (or raised some
        # other exception).
        raise exc  # pylint: disable=raising-bad-type
    raise TypeError("expected str, bytes, or bytearray not {}".format(type(s)))
