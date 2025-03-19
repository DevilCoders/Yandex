from setuptools import setup, find_packages

setup(
    name='Active Analytics',
    version='0.0.1',
    packages=find_packages('src'),
    package_dir={'': 'src'},
    test_suite='',
    url='',
    license='Yandex Cloud Analytics',
    zip_safe=True,
    install_requires=[]
)

