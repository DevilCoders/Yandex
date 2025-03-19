"""
Variables that influence testing behavior are defined here.
"""
import base64
import os
import string
from random import choice as random_choise

from argon2 import PasswordHasher
from nacl.encoding import URLSafeBase64Encoder as encoder
from nacl.public import Box, PrivateKey
from nacl.utils import random


def gen_keypair():
    """
    Generate new nacl key pair
    """
    pair = PrivateKey.generate()
    secret_str = pair.encode(encoder).decode('utf-8')
    public_str = pair.public_key.encode(encoder).decode('utf-8')
    keys = {
        'secret_obj': pair,
        'secret': secret_str,
        'public_obj': pair.public_key,
        'public': public_str,
    }
    return keys


def gen_random_string(length=64):
    """
    Generate random alphanum sequence + base64 symbols.
    """
    return base64.b64encode(os.urandom(length)).decode('utf-8')[:length]


def gen_plain_random_string(length=64):
    """
    Generate random alphanum sequence
    """
    return ''.join(random_choise(string.ascii_letters + string.digits) for _ in range(length))


def argon2(cleartext):
    """
    Return argon2 hash from cleartext.
    https://en.wikipedia.org/wiki/Argon2
    """
    hasher = PasswordHasher()
    return hasher.hash(cleartext)


class CryptoBox(Box):
    """
    Subclass Box to avoid boilerplating in encryption.
    Usually we just need an encrypted, encoded string,
    which is provided by the following method.
    """

    def encrypt_utf(self, data, *args, **kwargs):
        """
        Return UTF-8 string, using explicit nonce
        """
        assert isinstance(data, str), 'data argument must be a string'

        # This is a nonce, it *MUST* only be used once,
        # but it is not considered
        # secret and can be transmitted or stored alongside the ciphertext. A
        # good source of nonces are just sequences of 24 random bytes.
        nonce = random(self.NONCE_SIZE)
        # Raw encrypted byte sequence.
        raw = self.encrypt(data.encode('utf8'), nonce, *args, **kwargs)
        # Decode it to printable string.
        return encoder.encode(raw).decode('utf8')

    def decrypt_utf(self, data):
        """
        Decrypt a string and return plaintext as utf8
        """
        assert isinstance(data, str), 'data argument must be a string'

        raw = self.decrypt(data.encode('utf8'), None, encoder)
        return raw.decode('utf8')
