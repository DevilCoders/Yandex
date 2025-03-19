"""Setup.py for pingunoque"""
from setuptools import setup

setup(
    name="pingunoque",
    version="1",
    author="Migalin Daniil",
    author_email="dmiga@yandex-team.ru",
    description=("Small keepalived-like daemon"),
    license="GPL",
    url="https://github.yandex-team.ru/admins/pingunoque",
    packages=["pingunoque"],
    entry_points={
        'console_scripts': [
            'pingunoque=pingunoque.daemon:daemon',
        ],
    },
    install_requires=['pyyaml'],
)
