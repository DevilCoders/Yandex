from setuptools import setup

setup(
    name='ice',
    version='1.0',
    packages=['ice_client', 'abbreviation_client'],
    package_data={'ice_client': ['data/*']},
    scripts=['ice'],
    url='https://github.yandex-team.ru/yandex-icecream/ice',
    license='GPL',
    author='Pavel Pushkarev',
    author_email='paulus@yandex-team.ru',
    description='The icecream command line client',
)
