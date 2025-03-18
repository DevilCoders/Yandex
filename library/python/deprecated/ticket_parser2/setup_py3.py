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
    description='TVM2.0 Ticket Parser',
    include_package_data=True,
    name='ticket_parser2_py3',
    packages=find_packages(),
    package_data={
        'ticket_parser2_py3': ['ticket_parser2_pymodule.so', 'ticket_parser2_pymodule.pyd'],
    },
    install_requires=[
        'mock',
    ],
    version='2.5.0',
)
