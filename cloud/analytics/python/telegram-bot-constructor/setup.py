from setuptools import setup, find_packages


setup(
    name='tbc',
    version='0.1.20',
    packages=find_packages('src'),
    package_dir={'': 'src'},
    test_suite='tbc',
    url='https://github.com/alexeykarnachev/telegram-bot-constructor',
    author='Alexey Karnachev',
    author_email='alekseykarnachev@gmail.com',
    zip_safe=True,
    install_requires=['sqlalchemy==1.2.0b1', 'python-telegram-bot==6.1.0', 'PyYAML', 'transitions', 'blockchain']
)

if __name__ == '__main__':
    pass
