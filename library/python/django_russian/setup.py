from distutils.core import setup

setup(
    name='django-russian',
    description='Russian human formatting of dates, numbers etc. for Django templates',
    packages=['django_russian', 'django_russian.templatetags'],
    package_data={'django_russian': ['forms_noun.txt', 'forms_adjective.txt', 'forms_verb.txt']},
    version='0.22'
)
