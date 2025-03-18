from distutils.core import setup

setup(
    name='django-template-common',
    description='Utility and Yandex-specific tags for Django templates',
    packages=['django_template_common', 'django_template_common.templatetags'],
    package_data = {'django_template_common': ['templates/*']},
    install_requires=[
        'six',
        'django',
    ],
    version='1.94',
)
