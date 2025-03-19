"""setup"""
from setuptools import setup, find_packages

setup(
    name="log_processor",
    author="Artem Khokhlov",
    author_email="media-admin@yandex-team.ru",
    description=("tool for nginx tskv log processing"),
    license="GPL",
    url="https://github.yandex-team.ru/serg-turbo/py-log-processor",
    packages=find_packages(),
    scripts=[
        'log_processor.py',
        ],
    install_requires=[
        'file-read-backwards',
        'argparse',
    ],
)
