# coding: utf-8

from setuptools import setup, find_packages


setup(
    name='ids',
    version='1.3.73',
    packages=find_packages(
        exclude=(
            'tests',
        )
    ),
    install_requires=[
        'python-memcached>=1.53',
        'yenv>=0.4',
        # add libffi-dev and libssl-dev in your debian build depends
        'requests[security]',
        'six',
    ],
    extras_require={
        'abc': [],
        'at': [
            'lxml>=3.3.1',
        ],
        'calendar': [],
        'calendar_internal': [],
        'formatter': [],
        'gap': [],
        'jira': [
            'jira==0.21',
        ],
        'orange': [],
        'plan': [],
        'staff': [],
        'inflector': [],
        'startrek': [],
        'startrek2': [
            'startrek_client>=1.6.0',
        ],
        'uatraits': [],
        'geobase': [],
        'directory': [],
        'intrasearch': [],
    }
)
