"""
ID Generators
"""

import string
from abc import ABC, abstractmethod

from flask import current_app

from .crypto import gen_random_string


class IDGenerator(ABC):
    """
    Abstract ID generator
    """

    @abstractmethod
    def generate_id(self, id_type):
        """
        Generate new id for type
        """


class HostnameGenerator(ABC):
    """
    Abstract hostname generator
    """

    @abstractmethod
    def generate_hostname(self, prefix, suffix):
        """
        Generate hostname with prefix and suffix
        """


class YCIDGenerator(IDGenerator):
    """
    Yandex.Cloud id generator
    """

    def __init__(self, config):
        self.prefix = config['YC_ID_PREFIX']

    def generate_id(self, _):
        return '{prefix}{suffix}'.format(
            prefix=self.prefix, suffix=gen_random_string(17, string.ascii_lowercase.replace('wxyz', '') + string.digits)
        )


class RandomHostnameGenerator(HostnameGenerator):
    """
    Random hostname generator
    """

    def __init__(self, _):
        pass

    def generate_hostname(self, prefix, suffix):
        return '{prefix}{random}{suffix}'.format(
            prefix=prefix, random=gen_random_string(16, string.ascii_lowercase + string.digits), suffix=suffix
        )


def gen_id(id_type):
    """
    Generate id
    """
    generator = current_app.config['ID_GENERATOR'](current_app.config)
    return generator.generate_id(id_type)


def gen_hostname(prefix, suffix):
    """
    Generate hostname
    """
    generator = current_app.config['HOSTNAME_GENERATOR'](current_app.config)
    return generator.generate_hostname(prefix, suffix)
