from setuptools import setup, find_packages

setup(
    name='switter_grafana',
    version='1.0',
    packages=find_packages(),
    scripts=['switter2grafana'],
    package_data={
        '': ['main.yaml', 'logging.yaml', 'yandex.pem'],
    },
    url='https://github.yandex-team.ru/admins/switter2grafana',
    license='GPL',
    author='Pavel Pushkarev',
    author_email='paulus@yandex-team.ru',
    description='Converts switter to grafana',
)
