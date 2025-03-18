from setuptools import setup


setup(
    version='0.50',
    name='ylog',
    description='Yandex-specific Python logging utilities',
    install_requires=['six'],
    extras_require={
        'pytz': ['pytz'],
    },
    packages=[
        'ylog',
    ],
)
