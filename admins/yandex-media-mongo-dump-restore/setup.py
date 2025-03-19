"""setup"""
from setuptools import setup, find_packages

setup(
    name="dump_restore",
    version="0.1",
    author="Artem Khokhlov",
    author_email="media-admin@yandex-team.ru",
    description=("mongo dump and restore"),
    license="GPL",
    url="https://github.yandex-team.ru/admins/tokk-mongo-backup",
    packages=find_packages(),
    scripts=[
        'mongo-dump-restore.py',
        ],
    install_requires=[
        'dotmap',
        'pyyaml',
        'pymongo',
        'kazoo'
    ],
)
