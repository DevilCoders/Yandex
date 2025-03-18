# coding: utf-8
from setuptools import setup, find_packages, findall

import steam
import os

setup(
    name='steam-site-pack',
    description='[S]nippet [T]ools: [E]valuation, [A]ssessment, [M]etrics',
    version=steam.__version__,

    author='Ivan Kryukov',
    author_email='my34@yandex-team.ru',

    packages=find_packages(exclude=['*.hard', '*.local', '*.env', '*.features']),
    package_data={
        '': [f[len('steam/'):] for f in findall('steam')
             if os.path.splitext(f)[1] in (
                 '.po', '.css', '.js', '.html', '.sql',
                 '.png', '.gif', '.jpg', '.jpeg',
                 '.types', '.conf', '.txt'
             )] + ['core/hard/__init__.py', 'ui/env/__init__.py',
                   'steamctl', 'etc/ubic/steam-*']
    },
    entry_points={
        'console_scripts': ['steam-site = steam.manage:main', ],
    },

)
