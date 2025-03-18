from setuptools import setup, find_packages

setup(
    name='dir-sync',
    version='0.55',

    description='django application for data synchronization with Yandex directory',

    packages=find_packages(exclude=('tests',)),
    include_package_data=True,
    package_data={'': ['dir_data_sync/fixtures/initial_data.json']},
    install_requires=[
        'Django>=1.9',
        'django-model-utils>=2.5',
        'djangorestframework>=3.2.5',
        'celery>=4.2.2',
        'requests[security]>=2.18.4',
        'ylock>=0.40',
        'ylog>=0.46',
        'jsonfield>=2.0.2',
        'tvm2>=2.2',
        'yenv>=0.9',
    ]
)
