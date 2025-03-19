#!/usr/bin/env python3
# encoding: utf-8
"""
setup.py for DBaaS bootstrap
"""

from setuptools import setup, find_packages


REQUIREMENTS = [
    'dbaas-common',
    'dbaas-worker',
]

setup(
    name='dbaas-bootstrap',
    version='0.0.1',
    description='DBaaS Bootstrap script',
    license='Yandex License',
    zip_safe=False,
    platforms=['Linux', 'BSD', 'MacOS'],
    packages=find_packages('.', exclude=('test',)),
    package_dir={'dbaas_bootstrap': 'dbaas_bootstrap'},
    entry_points={
        'console_scripts': [
            'dbaas_bootstrap = dbaas_bootstrap:main',
    ]},
    install_requires=REQUIREMENTS,
)
