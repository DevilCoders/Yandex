
from setuptools import setup, find_packages
setup(
        name="pymonmongo",
        version="1.0",
        author="Pavel Pushkarev",
        author_email="media-admin@yandex-team.ru",
        description=("Monitor queries"),
        license="GPL",
        url="https://github.yandex-team.ru/admins/pymonmongo",
        packages=find_packages(),
        scripts=[
            'mongo-query-monitor',
            'mongo-collection-distribution',
            ],
        install_requires=[
            'pymongo',
            'termcolor',
            ],
        )
