from distutils.core import setup

setup(
    name='blackbox',
    version='0.72',
    description='Python interface to Blackbox facility',
    author='Ivan Chelyubeev',
    author_email='ijon@yandex-team.ru',
    py_modules=['blackbox'],
    install_requires=['six', 'yenv', 'tvm2>=5.0', 'retrying'],
)
