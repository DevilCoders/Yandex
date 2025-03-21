import io
import os
import re
import sys

from setuptools import setup, find_packages

PATH_BASE = os.path.dirname(__file__)


def read_file(fpath):
    """Reads a file within package directories."""
    with io.open(os.path.join(PATH_BASE, fpath)) as f:
        return f.read()


def get_version():
    """Returns version number, without module import (which can lead to ImportError
    if some dependencies are unavailable before install."""
    contents = read_file(os.path.join('refsclient', '__init__.py'))
    version = re.search('VERSION = \(([^)]+)\)', contents)
    version = version.group(1).replace(', ', '.').strip()
    return version


setup(
    name='refsclient',
    version=get_version(),
    url='https://github.yandex-team.ru/Billing/refsclient',

    description='Yandex refs service Python Client',
    long_description=read_file('README.rst'),

    packages=find_packages(),
    include_package_data=True,
    zip_safe=False,

    install_requires=[
        'requests',
    ],
    setup_requires=[] + (['pytest-runner'] if 'test' in sys.argv else []) + [],

    extras_require={
        'cli': ['click>=2.0'],
    },

    entry_points={
        'console_scripts': ['refsclient = refsclient.cli:main'],
    },
    test_suite='tests',

    tests_require=['pytest'],

    classifiers=[
        # As in https://pypi.python.org/pypi?:action=list_classifiers
        'Development Status :: 4 - Beta',
        'Operating System :: OS Independent',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: BSD License'
    ],
)
