from setuptools import setup, find_packages

setup(
    name='django_alive',
    version='1.1.12',

    description='Django library for complex application checks',
    packages=find_packages(exclude=['tests']),

    install_requires=[
        'Django>=1.4',
        'tzlocal',
        'six',
    ],
    extras_require={
        'db': ['django_replicated>=2.0'],
        'http': ['requests>=1'],
        'mongodb': ['pymongo'],
        'redis': ['redis'],
        'zookeeper': ['kazoo'],
        'memcached': ['python-memcached'],
    },

    include_package_data=True,
)
