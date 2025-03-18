from setuptools import setup, find_packages

setup(
    name='cia-stuff',
    version='1.1.3',
    description='Common stuff for CIA services',
    packages=find_packages(
        exclude=['tests']
    ),
    install_requires=[
        'arrow',
        'django',
        'pytz',
        'requests',
        'ylog>=0.47',
        'cached-property',
        'django_alive',
        'yenv',
    ],
    include_package_data=True,
)
