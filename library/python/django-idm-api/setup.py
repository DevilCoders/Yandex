# coding: utf-8
from __future__ import unicode_literals
from setuptools import setup, find_packages


setup(
    name='django-idm-api',
    version='2.14.0',
    description='Реализация HTTP API Управлятора для Джанго проектов.',
    long_description=(
        'Подробную информацию читайте на http://wiki.yandex-team.ru/intranet/idm/API/Django, '
        'о багах можно писать в Стартрек: to https://st.yandex-team.ru/RULES или на idm-dev@'
    ),
    author='Tools Team',
    author_email='idm-dev@yandex-team.ru',
    packages=find_packages(
        exclude=['tests'],
    ),
    install_requires=[
        'django>=1.3',
        'yenv>=0.5',
        'tvm2>=2.2',
        'attrs>=17.1.0',
    ],
)
