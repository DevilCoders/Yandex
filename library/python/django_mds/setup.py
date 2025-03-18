from setuptools import find_packages, setup


setup(
    name='django_mds',
    version='0.13.0',
    packages=find_packages(),

    author='Alexander Yuzhakov, Maxim Novikov',
    author_email='cmind@yandex-team.ru, nes8bit@yandex-team.ru',
    license='BSD',
    description='MDS file storage backend for Django.',
    url='http://github.yandex-team.ru/weather/django-mds',

    install_requires=[
        'six',
        'requests>=2.3',
        'Django',
        'tvm2>=2.4',
    ]
)
