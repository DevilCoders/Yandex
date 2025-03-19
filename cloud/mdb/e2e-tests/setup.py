from setuptools import setup, find_packages

setup(
    name='dbaas_e2e',
    version='0.0.1',
    packages=['dbaas_e2e', 'dbaas_e2e.scenarios'],
    description='DBaaS end to end tests',
    entry_points={
        'console_scripts': [
            'dbaas_e2e = dbaas_e2e:_main',
        ]
    },
    license='Yandex license',
    url='https://github.yandex-team.ru/mdb/dbaas-e2e/',
    install_requires=[
        'retrying',
        'requests',
        'psycopg2',
        'jsonschema',
        'clickhouse-driver',
        'pymongo',
    ],
    author_email='mdb-admin@yandex-team.ru',
)
