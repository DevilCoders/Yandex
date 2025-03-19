from setuptools import setup, find_packages

setup(
    name='generate_awacs_config',
    version='1.0',
    url='https://github.com/riarheos/abbreviation_client',
    license='GPL',
    author='Pavel Pushkarev',
    author_email='paulus@yandex-team.ru',
    description='The awacs config generator',
    packages=find_packages(),
    scripts=['generate-awacs-config'],
    package_data={
        '': ['templates', 'logging.yaml', 'generate-awacs-config.1'],
    },
    install_requires=['jinja2', 'pyyaml', 'requests'],
)
