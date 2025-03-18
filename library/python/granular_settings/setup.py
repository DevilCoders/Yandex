# -*- coding: utf-8 -*-

from setuptools import setup, find_packages

setup(
    name='granular_settings',
    version='1.5',
    packages=['granular_settings'],
    author='Mikhail Petrov',
    author_email='mixael@yandex-team.ru',
    include_package_data=True,
    install_requires=['yenv>=0.5', 'six']
)
