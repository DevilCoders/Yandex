from setuptools import setup, find_packages

setup(
    name='django_yauth',
    version='5.3',

    description='django application for Yandex authentication',
    packages=find_packages(),
    package_data={'django_yauth': ['css/*.css', 'templates/*.html']},
    install_requires=[
        'Django>=1.3',
        'blackbox<1.0',
        'yenv>=0.4',
        'django-template-common>=1.93',
        'six',
        'tvm2>=5.0',
    ]
)
