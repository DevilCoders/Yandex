#!/usr/bin/env python3
# encoding: utf-8
"""
setup.py for mdbui
"""

try:
    from setuptools import setup, find_packages
except ImportError:
    from distutils import setup

REQUIREMENTS = [
    'psycopg2-binary',
    'Django',
    'pytils',
    'django-nested-inline',
    'webencodings',
    'django-js-asset',
    'django-jinja',
    'django-postgrespool2',
    'django-csp',
    'dictdiffer',
    'blackboxer',
    'tvm2',
    'uwsgi',
    'sentry-sdk',
]

setup(
    name='mdbui',
    version='0.0.1',
    description='MDB UI',
    license='Yandex License',
    url='https://github.yandex-team.ru/mdb/ui/',
    author='Anna Krkhanbarova',
    author_email='annkpx@yandex-team.ru',
    maintainer='Anna Krkhanbarova',
    maintainer_email='annkpx@yandex-team.ru',
    zip_safe=False,
    platforms=['Linux', 'BSD', 'MacOS'],
    packages=['mdbui'],
    package_dir={'mdbui': 'src'},
    install_requires=REQUIREMENTS,
    include_package_data=True,
    entry_points={
        'console_scripts': [
            'mdbui = manage:management',
        ],
    },
)
