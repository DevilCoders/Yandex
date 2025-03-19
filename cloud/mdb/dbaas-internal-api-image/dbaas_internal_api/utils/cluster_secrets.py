# -*- coding: utf-8 -*-
"""
Cluster secrets provider
"""

from abc import ABC, abstractmethod
from typing import Tuple

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from flask import current_app


class ClusterSecretsProvider(ABC):
    """
    Abstract Cluster Secrets Provider
    """

    @abstractmethod
    def generate(self) -> Tuple[bytes, bytes]:
        """
        Generate private and public keys for cluster
        """


class RSAClusterSecretsProvider(ClusterSecretsProvider):
    """
    RSA Cluster Secrets Provider
    """

    def __init__(self, config):
        self.exponent = config['RSA_CLUSTER_SECRETS']['exponent']
        self.key_size = config['RSA_CLUSTER_SECRETS']['key_size']

    def generate(self) -> Tuple[bytes, bytes]:
        """
        Generates private and public keys for cluster, returns them in PEM format.
        """
        private_key = rsa.generate_private_key(
            public_exponent=self.exponent, key_size=self.key_size, backend=default_backend()
        )
        public_key = private_key.public_key()

        private_key_pem = private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption(),
        )
        public_key_pem = public_key.public_bytes(
            encoding=serialization.Encoding.PEM, format=serialization.PublicFormat.PKCS1
        )

        return private_key_pem, public_key_pem


def generate_cluster_keys() -> Tuple[bytes, bytes]:
    """
    Generate private and public keys for cluster using provider defined in config
    """
    provider = current_app.config['CLUSTER_SECRETS_PROVIDER'](current_app.config)
    return provider.generate()
