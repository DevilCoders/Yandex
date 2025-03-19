"""
Crypto functions
"""

from nacl.encoding import URLSafeBase64Encoder as encoder
from nacl.public import Box, PrivateKey, PublicKey
from nacl.utils import random


def encrypt(config, data):
    """
    Encrypt data and return dict with embedded encryption version
    """
    pub_key = PublicKey(config.main.client_pub_key.encode('utf-8'), encoder)
    sec_key = PrivateKey(config.main.api_sec_key.encode('utf-8'), encoder)
    box = Box(sec_key, pub_key)

    encrypted = encoder.encode(box.encrypt(data.encode('utf-8'), random(box.NONCE_SIZE))).decode('utf-8')

    return {'encryption_version': 1, 'data': encrypted}


def decrypt(config, data: dict) -> str:
    """
    Decrypt encrypted string
    """

    pub_key = PublicKey(config.main.client_pub_key.encode('utf-8'), encoder)
    sec_key = PrivateKey(config.main.api_sec_key.encode('utf-8'), encoder)
    box = Box(sec_key, pub_key)

    decrypted = box.decrypt(encoder.decode(data['data'].encode('utf-8'))).decode('utf-8')

    return decrypted
