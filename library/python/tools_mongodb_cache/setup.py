# -*- coding: utf-8 -*-

from setuptools import setup, find_packages
setup(
    name='tools_mongodb_cache',
    version='2.6',
    packages=find_packages(),
    include_package_data=True,
    author='Anton Chaporgin',
    author_email='chapson@yandex-team.ru',
    description='Useful to have consistent and simple cache in multi-datacenter installations. See github README.',
    url='git@github.com:antonyc/mongodb_cache.git',
)
