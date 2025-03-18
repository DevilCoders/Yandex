# coding: utf-8

from __future__ import unicode_literals

from setuptools import setup, find_packages

setup(
    name='django_tools_log_context',
    version='1.1.1',

    description=(
        'https://a.yandex-team.ru/arc/trunk/arcadia/library/python/django_tools_log_context'
    ),

    packages=find_packages(exclude=('tests',)),
    install_requires=[
        'Django',
        'celery',
        'ylog>=0.47',
        'cached_property',
        'monotonic',
        'six',
    ]
)
