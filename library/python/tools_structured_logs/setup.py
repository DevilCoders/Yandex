# coding: utf-8

from setuptools import setup, find_packages

setup(
    name='tools_structured_logs',
    version='0.16',

    description=(
        'https://a.yandex-team.ru/arc/trunk/arcadia/library/python/tools_structured_logs'
    ),

    packages=find_packages(exclude=('tests',)),
    install_requires=[
        'cached_property',
        'monotonic',
        'six',
    ]
)
