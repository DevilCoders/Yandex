# coding: utf-8
from __future__ import unicode_literals
from setuptools import setup, find_packages


setup(
    name='django-abc-data',
    version='0.2.16',
    description='Приложение для синхронизации ABC-сервисов.',
    author='Tools Team',
    author_email='tools@yandex-team.ru',
    packages=find_packages(),
    install_requires=['six',
                      'sync-tools',
                      'django-appconf',
                      'ids[abc]',
                      'django-closuretree',
                      ],
)
