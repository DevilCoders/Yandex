from distutils.core import setup

import django_tanker


setup(
    name='django-tanker',
    version=django_tanker.__version__,

    author='Alex Koshelev',
    author_email='alexkoshelev@yandex-team.ru',

    description='Django application for Tanker integration',

    packages=[
        'django_tanker',
        'django_tanker.management',
        'django_tanker.management.commands',
    ],
    install_requires=[
        'django',
        'lxml',
        'requests',
    ],
    scripts=['scripts/django-tanker.py'],
)
