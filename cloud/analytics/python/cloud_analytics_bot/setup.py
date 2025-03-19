from setuptools import setup, find_packages

setup(
    name='cloud_analytics_bot',
    version='0.0.1',
    packages=find_packages('src'),
    package_dir={'': 'src'},
    test_suite='cloud_analytics_bot',
    url='',
    license='Siemens CT RDA BAM COM RU proprietary license',
    author='Anton Bakuteev',
    author_email='bakuteev@yandex-team.ru',
    zip_safe=True,
    install_requires=[]
)
