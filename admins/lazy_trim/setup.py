#!/usr/bin/env python2
# encoding: utf-8
"""
setup.py for lazy-trim
"""

import os

try:
    from setuptools import setup
except ImportError:
    from distutils import setup


REQUIREMENTS = [
    'lockfile',
]

setup(
    name='lazy-trim',
    version='0.1.0',
    description='small utility for thottled trimming ssd disks',
    license='Yandex License',
    url='https://github.yandex-team.ru/mdb/lazy-trim',
    author='MDB Team',
    author_email='mdb@yandex-team.ru',
    maintainer='MDB Team',
    maintainer_email='mdb@yandex-team.ru',
    zip_safe=False,
    platforms=['Linux'],
    packages=['lazy_trim'],
    package_dir={
        'lazy_trim': 'lazy_trim',
    },
    entry_points={'console_scripts': [
        'lazy_trim = lazy_trim.main:main',
    ]},
    install_requires=REQUIREMENTS,
)
