from setuptools import setup, find_packages


setup(
    name='iptables_except_my_group',
    version='0.0.3',
    description='Close/open machine from world except conductor group members',
    author='Alet team',
    author_email='cult-admin@yandex-team.ru',
    url='https://github.yandex-team.ru/admins/pypitables',
    packages=['iptables_except_my_group'],
    package_dir={'':'src'},
    python_requires='>=3',
    install_requires=[
        'setuptools>=18.5',
        'requests>=2.19.1',
        'requests-cache',
        'cryptography>=2.2.1',
        'pyOpenSSL>=0.14',
        'cffi>=1.7',
        'wheel'
    ],
    entry_points="""
    [console_scripts]
    iptables_except_my_group = iptables_except_my_group.iptables_except_my_group:main
    react_mongo_lag = iptables_except_my_group.react_mongo_lag:main
    """
)
