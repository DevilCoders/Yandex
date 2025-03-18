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
    description='TVM client',
    include_package_data=True,
    name='tvmauth',
    packages=find_packages(),
    package_data={
        'tvmauth': ['tvmauth_pymodule.so', 'tvmauth_pymodule.pyd'],
    },
    install_requires=[
        'mock',
        'six',
        'urllib3',
    ],
    version='3.4.5',
)
