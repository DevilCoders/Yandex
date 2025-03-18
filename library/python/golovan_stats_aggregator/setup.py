# -*- coding: utf-8 -*-
from setuptools import setup


version = '2.0.0'

setup(
    name='golovan_stats_aggregator',
    version=version,
    url='https://github.yandex-team.ru/workspace/golovan-stats-aggregator',
    description='',
    author='Nail Khunafin',
    author_email='khunafin@yandex-team.ru',
    packages=['golovan_stats_aggregator'],
    install_requires=[
        'six'
    ],
    test_suite='tests',
)
