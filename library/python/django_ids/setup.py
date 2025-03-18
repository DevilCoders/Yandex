from setuptools import setup, find_packages

setup(
    name='django-ids',
    version='0.10',
    description='More better django-intranet-stuff',
    packages=find_packages(
    ),
    install_requires=[
        'ids',
        'django',
    ],
    include_package_data=True,
)

