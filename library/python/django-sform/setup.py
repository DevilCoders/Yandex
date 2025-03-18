from distutils.core import setup

setup(
    name='django-sform',
    version='2.1',
    packages=['sform'],
    install_requires=[
        'django>=1.8',
        'six',
    ],
)
