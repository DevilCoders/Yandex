import setuptools


def main():
    setuptools.setup(
        name='yandex-blender-factor-storage',
        version='1.0',
        packages=setuptools.find_packages(),
        include_package_data=True,
        package_data={
            'kernel.blender.factor_storage.pylib': [ 'compression.so' ],
        },
        author='dima-zakharov',
        author_email='dima-zakharov@yandex-team.ru',
        url='https://a.yandex-team.ru/arc/trunk/arcadia/kernel/blender/factor_storage',
   )


if __name__ == '__main__':
    main()
