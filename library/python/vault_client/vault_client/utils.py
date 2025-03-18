# -*- coding: utf-8 -*-

from base64 import urlsafe_b64encode
import binascii
import hashlib
import os

from paramiko.message import Message
import six


def noneless_dict(d):
    return {k: v for k, v in d.items() if v is not None}


def _message_to_bytes(msg):
    if isinstance(msg, Message):
        return msg.asbytes()
    else:
        return bytes(msg)


def sign_in_memory(data, key):
    return urlsafe_b64encode(
        _message_to_bytes(
            key.sign_ssh_data(six.b(data)),
        ),
    )


def default_hash(value):
    return hashlib.sha256(value).hexdigest()


def merge_dicts(d1, *args):
    merged = dict(d1 or {})
    for d in args:
        if d is not None:
            merged.update(d)
    return merged


def token_bytes(nbytes=32):  # pragma: no cover
    """Return a random byte string containing *nbytes* bytes.
    If *nbytes* is ``None`` or not supplied, a reasonable
    default is used.
    >>> token_bytes(16)  #doctest:+SKIP
    b'\\xebr\\x17D*t\\xae\\xd4\\xe3S\\xb6\\xe2\\xebP1\\x8b'
    """
    if nbytes is None:
        nbytes = 32
    return os.urandom(nbytes)


def token_hex(nbytes=None):
    """Return a random text string, in hexadecimal.

    The string has *nbytes* random bytes, each byte converted to two
    hex digits.  If *nbytes* is ``None`` or not supplied, a reasonable
    default is used.

    >>> token_hex(16)  #doctest:+SKIP
    'f9bf78b9a18ce6d46a0cd2b0b86df9da'

    """
    return binascii.hexlify(token_bytes(nbytes)).decode('ascii')
