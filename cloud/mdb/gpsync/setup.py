#!/usr/bin/env python
# encoding: utf-8

from setuptools import setup

setup(
    name='gpsync',
    version='3.0',
    author='Vladimir Borodin',
    author_email='d0uble@yandex-team.ru',
    url='https://github.yandex-team.ru/mail-admin/gpsync',
    description="Automatic failover of PostgreSQL with help of ZK",
    long_description="Automatic failover of PostgreSQL with help of ZK",
    license="PostgreSQL",
    platforms=["Linux", "BSD", "MacOS"],
    zip_safe=False,
    packages=['gpsync'],
    package_dir={'gpsync': 'src'},
    package_data={'gpsync': ['src/plugins/', 'plugins/*.py']},
    entry_points={
        'console_scripts': [
            'gpsync = gpsync:main',
        ]
    },
    scripts=["bin/gpsync-util"],
)
