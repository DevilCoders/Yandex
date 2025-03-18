#!/usr/bin/python

from setuptools import setup, find_packages


setup(
    name='ylock',
    version='0.44',
    author='Morozova Anastasia',
    author_email='scullyx13@yandex-team.ru',
    description='Python client library for distributed lock',

    packages=find_packages(exclude=['tests']),
    install_requires=['yenv', 'click'],

    extras_require={
        'zookeeper': ['kazoo>=1.3.1'],
        'mongodb': ['pymongo'],
        'yt': ['yandex-yt>=0.8.49'],
    },

    entry_points={
        'console_scripts': [
           'ylock = ylock.main:main',
        ],
    },
)
