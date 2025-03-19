from setuptools import setup, find_packages

setup(
    name='support_tickets_classification',
    version='0.2.11',
    packages=find_packages('src'),
    package_dir={'': 'src'},
    test_suite='',
    entry_points={
        'console_scripts': ['classify_endpoint=support_tickets_classification.cli.classify_endpoint:main'],
    },
    license='Yandex Cloud Analytics',
    zip_safe=True,
    package_data={
    'support_tickets_classification.data': ['*', '*/*', '*/*/*', '*/*/*/*'],
    }
)
