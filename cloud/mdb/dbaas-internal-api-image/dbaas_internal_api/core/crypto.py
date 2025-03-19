# -*- coding: utf-8 -*-
"""
DBaaS Internal REST API crypto functions
"""

import string
import hashlib
from abc import ABC, abstractmethod
from secrets import choice

from flask import current_app
from nacl.encoding import URLSafeBase64Encoder as encoder
from nacl.public import Box, PrivateKey, PublicKey
from nacl.utils import random

from ..utils.config import get_password_length


class CryptoProvider(ABC):
    """
    Abstract Crypto provider
    """

    @abstractmethod
    def encrypt(self, data: str):
        """
        Encrypt str data
        """

    @abstractmethod
    def encrypt_bytes(self, data: bytes):
        """
        Encrypt bytes data
        """

    @abstractmethod
    def gen_random_string(self, length, symbols):
        """
        Generate random string
        """


class NaCLCryptoProvider(CryptoProvider):
    """
    NaCL-based crypto provider
    """

    def __init__(self, config):
        pub_key = PublicKey(config['CRYPTO']['client_public_key'].encode('utf-8'), encoder)
        sec_key = PrivateKey(config['CRYPTO']['api_secret_key'].encode('utf-8'), encoder)
        self.box = Box(sec_key, pub_key)

    def encrypt(self, data: str) -> dict:
        return self.encrypt_bytes(data.encode('utf-8'))

    def encrypt_bytes(self, data: bytes) -> dict:
        encrypted = encoder.encode(self.box.encrypt(data, random(self.box.NONCE_SIZE))).decode('utf-8')

        return {'encryption_version': 1, 'data': encrypted}

    def gen_random_string(self, length, symbols=(string.ascii_letters + string.digits)):
        """
        Generate random string
        """
        return ''.join([choice(symbols) for _ in range(length)])  # nosec


def encrypt(data: str):
    """
    Encrypt string data with crypto provider
    """
    provider = current_app.config['CRYPTO_PROVIDER'](current_app.config)
    return provider.encrypt(data)


def encrypt_bytes(data: bytes):
    """
    Encrypt bytes data with crypto provider
    """
    provider = current_app.config['CRYPTO_PROVIDER'](current_app.config)
    return provider.encrypt_bytes(data)


def gen_random_string(length, symbols=(string.ascii_letters + string.digits)):
    """
    Generate random string with crypto provider
    """
    provider = current_app.config['CRYPTO_PROVIDER'](current_app.config)
    return provider.gen_random_string(length, symbols)


def gen_encrypted_password(length: int = None):
    """
    Generate password and encrypt it with encrypt function
    """
    if length is None:
        length = get_password_length()

    return encrypt(gen_random_string(length))


def gen_encrypted_password_with_hash(length: int = None):
    """
    Generate password and encrypt it with encrypt function
    """
    if length is None:
        length = get_password_length()

    password = gen_random_string(length)
    hash = hashlib.sha256(password.encode('utf-8')).hexdigest()
    return encrypt(password), encrypt(hash)
