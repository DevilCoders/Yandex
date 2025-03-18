from setuptools import setup, find_packages


setup(
    name='tvm2',
    version='5.0',
    author='Vladimir Koljasinskij',
    author_email='smosker@yandex-team.ru',
    url='https://a.yandex-team.ru/arc/trunk/arcadia/library/python/tvm2',
    description='library for serving tvm tickets, tvmauth wrapper',
    packages=find_packages(),
    install_requires=[
        'requests>=2.18.4',
        'tvmauth>=3.1.0',
        'ylog>=0.47',
        'yenv>=0.8',
        'blackbox>=0.70',
        'aiohttp;python_version>="3"',
        'tenacity>=6.2.0;python_version>="3"',
    ],
)
