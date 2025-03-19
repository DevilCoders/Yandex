#!/usr/bin/env python3.6
# encoding: utf-8
"""
setup.py for DBaaS mdb-mongo-tools
"""

from setuptools import setup, find_packages


REQUIREMENTS = [
    'PyYAML',
    'pymongo',
    'tenacity',
    'filelock',
]

setup(
    name='mdb-mongo-tools',
    version='0.0.1',
    description='MDB MongoDB tools',
    license='Yandex License',
    url='https://github.yandex-team.ru/mdb/mdb-mongo-tools',
    author='DBaaS team',
    author_email='mdb-admin@yandex-team.ru',
    maintainer='MDB team',
    maintainer_email='mdb-admin@yandex-team.ru',
    zip_safe=False,
    platforms=['Linux', 'BSD', 'MacOS'],
    packages=find_packages(exclude=['tests*']),
    entry_points={'console_scripts': [
        'mdb-mongod-resetup = mdb_mongo_tools.scripts.mongod_resetup:main',
        'mdb-mongod-stepdown = mdb_mongo_tools.scripts.mongod_stepdown:main',
        'mdb-mongo-get = mdb_mongo_tools.scripts.mongo_get:main',
    ]},
    install_requires=REQUIREMENTS,
)
