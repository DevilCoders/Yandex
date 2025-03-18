import os
from subprocess import check_output, CalledProcessError
from distutils.core import setup

MAJOR = 2
MINOR = 4
PATCH = 0
ISRELEASED = False
VERSION = '{}.{}.{}'.format(MAJOR, MINOR, PATCH)


packages = {
    'nirvana_api': '.',
    'nirvana_api.blocks': 'blocks',
}


def get_svn_revision():
    try:
        svn_revision = check_output('svn info | grep "Revision"', shell=True).strip()
        return svn_revision[len('Revision:'):].strip()
    except (OSError, CalledProcessError):
        return None


def write_version_py(version, full_version, svn_revision, filename='version.py'):
    version_py = """
# THIS FILE IS GENERATED FROM SETUP.PY
version = '{version}'
full_version = '{full_version}'
svn_revision = '{svn_revision}'
"""
    with open(filename, 'w') as stream:
        stream.write(version_py.format(version=version, full_version=full_version, svn_revision=svn_revision))


def read_svn_revision_from_version_py(filename='version.py'):
    if not os.path.exists(filename):
        return None
    import imp
    version = imp.load_source('nirvana_api.version', filename)
    return version.svn_revision


def setup_package():
    svn_revision = os.getenv('SVN_REVISION')
    if not svn_revision:
        svn_revision = get_svn_revision()
    if not svn_revision:
        svn_revision = read_svn_revision_from_version_py()
    full_version = VERSION if ISRELEASED else '{}.post{}'.format(VERSION, svn_revision)
    write_version_py(VERSION, full_version, svn_revision)

    setup(
        name='nirvana-api',
        version=full_version,
        author='mihajlova',
        author_email='mihajlova@yandex-team.ru',
        url='https://a.yandex-team.ru/arc/trunk/arcadia/library/python/nirvana_api',
        packages=packages,
        package_dir=packages,
        description='Nirvana API',
        install_requires=[
            'enum34',
            'graphviz',
            'requests',
            'six',
        ],
    )


if __name__ == '__main__':
    setup_package()
