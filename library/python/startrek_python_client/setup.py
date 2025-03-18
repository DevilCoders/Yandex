# coding: utf-8
import sys
from setuptools import setup

if sys.version_info.major < 3:
    stools = 'setuptools<=42.0.2'
else:
    stools = 'setuptools'

setup(
    name='startrek_client',
    version='2.5',
    description='Client for Startrek (with OAuth authorization)',
    author='Tools Team',
    author_email='tools-dev@yandex-team.ru',
    url='https://github.yandex-team.ru/tools/startrek-python-client',
    packages=['startrek_client'],
    install_requires=[
        'requests[security]>=2.0',
        stools,
        'six>=1.9',
        'yandex_tracker_client',
    ]
)
