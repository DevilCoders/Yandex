#!/usr/bin/env python
# encoding: utf-8

from setuptools import setup

setup(
    name='pgsync',
    version='3.0',
    author='Vladimir Borodin',
    author_email='d0uble@yandex-team.ru',
    url='https://github.yandex-team.ru/mail-admin/pgsync',
    description="Automatic failover of PostgreSQL with help of ZK",
    long_description="Automatic failover of PostgreSQL with help of ZK",
    license="PostgreSQL",
    platforms=["Linux", "BSD", "MacOS"],
    zip_safe=False,
    packages=['pgsync'],
    package_dir={'pgsync': 'src'},
    package_data={'pgsync': ['src/plugins/', 'plugins/*.py']},
    entry_points={
        'console_scripts': [
            'pgsync = pgsync:main',
        ]
    },
    scripts=["bin/pgsync-util"],
)
