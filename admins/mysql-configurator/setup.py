"""
setup virtualenv
"""
from setuptools import setup, find_packages
setup(
    name="mysql-configurator",
    version="4",
    author="Artem Khokhlov, Pavel Pushkarev, Sergey Kacheev",
    author_email="media-admin@yandex-team.ru",
    description=("The python version of the MySQL configurator"),
    license="GPL",
    url="https://github.yandex-team.ru/admins/mysql-configurator",
    packages=find_packages(),
    scripts=[
        'mysql_monitoring_daemon',
        'mysql_monitoring_slow_query',
        'mysql_replace_configs.py',
        'mysql_backup_data',
        'mysql_backup_binlogs',
        'mysql_purge_binlogs',
        'mysql_grants_update.py',
        'mysql_transaction_killer',
        'mysql_replica_watcher',
        ]
)
