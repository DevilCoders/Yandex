# coding: utf-8
from __future__ import unicode_literals

from setuptools import setup, find_packages

setup(
    name='metrics_framework',
    version='0.0.7',

    description='https://github.yandex-team.ru/idm/metrics_framework',

    packages=find_packages(),
    install_requires=[
        'Django',
        'celery',
        'ylock',
        'yenv',
        'python_statface_client',
    ]
)
