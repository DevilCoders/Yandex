# encoding: utf-8

from setuptools import setup, find_packages


requirements = [
    'Django>=1.3',
    'simplejson',
    'django-filter',
]

setup(
    name="django-multic",
    version="1.3.0",
    packages=['multic'],
    url="http://wiki.yandex-team.ru/intranet/center/multicomplete",
    description="Backend for multicomplete",
    install_requires=requirements,
)
