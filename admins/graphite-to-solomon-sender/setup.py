"""setup"""
from setuptools import setup, find_packages

setup(
    name="graphite_to_solomon_sender",
    author="Sergey Eliseev",
    author_email="media-admin@yandex-team.ru",
    description=("graphite to solomon sender"),
    license="GPL",
    url="https://github.yandex-team.ru/admins/graphite-to-solomon-sender",
    packages=find_packages(),
    scripts=[
        'solomon_sender.py',
        ],
    install_requires=[
        'tornado==5.1.1',
        'argparse'
    ],
)
