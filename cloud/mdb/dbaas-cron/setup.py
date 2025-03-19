#!/usr/bin/env python3
# encoding: utf-8
"""
setup.py for DBaaS Container scheduler
"""

try:
    from setuptools import setup
except ImportError:
    from distutils import setup

REQUIREMENTS = [
    'APScheduler',
    'jsonschema',
    'python-dateutil',
]

setup(
    name="dbaas-cron",
    version="0.0.1",
    description="DBaaS Container scheduler",
    license="Yandex License",
    url="https://github.yandex-team.ru/mdb/dbaas-cron/",
    author='Evgeny Dyukov',
    author_email='secwall@yandex-team.ru',
    maintainer='Evgeny Dyukov',
    maintainer_email='secwall@yandex-team.ru',
    zip_safe=False,
    platforms=["Linux", "BSD", "MacOS"],
    packages=['.'],
    entry_points={'console_scripts': ['dbaas_cron = cron:_main']},
    install_requires=REQUIREMENTS,
)
