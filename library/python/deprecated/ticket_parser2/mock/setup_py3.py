from setuptools import (
    find_packages,
    setup,
)


try:
    from wheel.bdist_wheel import bdist_wheel

    class _bdist_wheel(bdist_wheel):
        def get_tag(self):
            tag = bdist_wheel.get_tag(self)
            return tag[0], tag[1], self.plat_name

    cmdclass = {'bdist_wheel': _bdist_wheel}
except ImportError:
    cmdclass = {}

setup(
    author='Passport Infra',
    author_email='passport-dev@yandex-team.ru',
    cmdclass=cmdclass,
    description='TVM2.0 Ticket Parser Mocks',
    name='ticket_parser2_py3_mock',
    packages=find_packages(),
    install_requires=[
        'mock',
        'ticket_parser2_py3',
    ],
    version='2.5.0',
)
