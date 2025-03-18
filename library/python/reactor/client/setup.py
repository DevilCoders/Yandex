from setuptools import setup, find_packages
from os import path

from io import open

here = path.abspath(path.dirname(__file__))


def get_long_description():
    with open(path.join(here, 'README.md'), encoding='utf-8') as f:
        return f.read()

setup(
    name='reactor_client',
    version='1.0.11',
    description='Python client for reactor http api',
    long_description=get_long_description(),
    long_description_content_type='text/markdown',
    url='https://a.yandex-team.ru/arc/trunk/arcadia/library/python/reactor/client',

    classifiers=[
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
    ],
    packages=find_packages(exclude=['contrib', 'docs', 'tests']),
    python_requires='>=2.7, <4',

    install_requires=[
        'enum34;python_version<"3.4"',
        'requests',
        'retry >= 0.9.0',
        'six',
        'marshmallow >= 2.19.0, < 3.0;python_version<"3.0"',
        'marshmallow >= 3.0;python_version>="3.0"'
    ],

    project_urls={
        'Source and readme': 'https://a.yandex-team.ru/arc/trunk/arcadia/library/python/reactor/client',
    },
)
