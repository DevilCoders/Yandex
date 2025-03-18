from setuptools import setup, find_packages

setup(
    name='django-celery-monitoring',
    description=(
        'https://a.yandex-team.ru/arc/trunk/arcadia/library/python/django_celery_monitoring'
    ),
    author='Kirill Kartashov',
    author_email='qazaq@yandex-team.ru',
    version='0.5.2',
    packages=find_packages(),
    install_requires=[
        'django>=1.11',
        'celery',
        'django_celery_results==1.1.2',
    ],
    entry_points={
        'celery.result_backends': [
            'django-db-ext = django_celery_monitoring.backends:DatabaseExtendedBackend',
        ],
    },
)
