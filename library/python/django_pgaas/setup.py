# coding: utf-8
from setuptools import setup, find_packages
from sys import version_info


setup(
    name='django_pgaas',
    description='PGaaS-specific database backend for Django',
    long_description=open('README.md', 'rb').read().decode('utf-8'),

    author='Maksim Kuznetcov',
    author_email='mkznts@yandex-team.ru',

    version='0.7.4',

    install_requires=[
        'django',
        'six',
        'django-appconf' + ('==1.0.3' if version_info[0] == 2 else ''),
        'psycopg2',
    ],
    extras_require={
        'zdm':  [
            'zero_downtime_migrations'
        ],
    },
    packages=find_packages(exclude=('tests', 'tests.*')),
)
