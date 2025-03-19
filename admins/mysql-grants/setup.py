from setuptools import setup, find_packages

setup(
    name="mysql-grants",
    version="4",
    author="Artem Khokhlov, Pavel Pushkarev",
    author_email="media-admin@yandex-team.ru",
    description=("The python version of the MySQL grants-update"),
    license="GPL",
    url="https://github.yandex-team.ru/admins/mysql-grants",
    packages=find_packages(),
    scripts=[
        'mysql-grants-update.py',
        ],
    install_requires=[
        'mysql-python',
        'netaddr',
        'requests',
        'GitPython',
        ],
)
