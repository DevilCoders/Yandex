#!/usr/bin/env python
# -*- coding: utf-8 -*-

try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup

setup(
    name='solomon',
    version='1.8',
    description='Solomon python client',
    long_description=__doc__,
    author='carabas',
    author_email='carabas@yandex-team.ru',
    packages=['solomon'],
    install_requires=[
        'furl>=1,<2',
        'requests>=2,<3'
    ]
)
