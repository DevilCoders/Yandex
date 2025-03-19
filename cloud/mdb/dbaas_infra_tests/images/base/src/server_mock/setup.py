#!/usr/bin/env python3
# encoding: utf-8
"""
setup.py for server_mock
"""

try:
    from setuptools import setup
except ImportError:
    from distutils import setup

REQUIREMENTS = [
    'Flask',
]

setup(
    name='server_mock',
    version='0.1.0',
    description='Flask application implementing base functionality of'
    ' web service mock.',
    license='Yandex License',
    url='https://github.yandex-team.ru/mdb/dbaas-infrastructure-test/'
    'tree/master/images/base/src/server_mock/',
    author='Alexander Burmak',
    author_email='alex-burmak@yandex-team.ru',
    maintainer_email='mdb-admin@yandex-team.ru',
    zip_safe=False,
    platforms=['Linux', 'BSD', 'MacOS'],
    packages=['server_mock'],
    install_requires=REQUIREMENTS,
)
