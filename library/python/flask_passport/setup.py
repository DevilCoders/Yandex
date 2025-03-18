# -*- coding: utf-8 -*-
from __future__ import absolute_import, division, print_function, unicode_literals

"""
Flask-Passport
------------
"""

from setuptools import setup

setup(
    name='Flask-Passport',
    version='0.1.0',
    url='',
    author='John Khvatov',
    author_email='ivaxer@yandex-team.ru',
    description='Passport authentication for yandex-team.tld domain',
    long_description=__doc__,
    py_modules=['flask_passport'],
    zip_safe=False,
    include_package_data=True,
    platforms='any',
    install_requires=['Flask-Principal', 'Flask', 'cachetools', 'requests', 'six'],
)
