import io
import os
import re
import sys
from itertools import chain

from setuptools import setup, find_packages

PATH_BASE = os.path.dirname(__file__)


def read(fpath):
    with io.open(fpath) as f:
        return f.read()


def get_version():
    contents = read(os.path.join(PATH_BASE, 'dssclient', '__init__.py'))
    version = re.search('VERSION = \(([^)]+)\)', contents)
    version = version.group(1).replace(', ', '.').strip()
    return version


extras = {
    'cli': ['click>=2.0'],
    'asn': ['pyasn1', 'pyasn1-modules'],
}
extras['all'] = list(chain.from_iterable(extras.values()))

setup(
    name='dssclient',
    version=get_version(),
    url='https://github.yandex-team.ru/Billing/dssclient',

    description='CryptoPro DSS Python Client',
    long_description=read(os.path.join(PATH_BASE, 'README.rst')),

    packages=find_packages(),
    include_package_data=True,
    zip_safe=False,

    install_requires=[
        'requests',
    ],
    setup_requires=[] + (['pytest-runner'] if 'test' in sys.argv else []) + [],

    extras_require=extras,

    entry_points={
        'console_scripts': ['dssclient = dssclient.cli:main'],
    },
    test_suite='tests',

    tests_require=['pytest'],

    classifiers=[
        # As in https://pypi.python.org/pypi?:action=list_classifiers
        'Development Status :: 4 - Beta',
        'Operating System :: OS Independent',
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: BSD License'
    ],
)


