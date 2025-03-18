# coding: utf-8

from __future__ import print_function

import os

from setuptools import setup, find_packages, dist
distmgr = dist.Distribution()


def mk_hook_command(cmd, func, func_post=None, cmd_cls_orig=None):
    cmd_cls_orig = cmd_cls_orig or distmgr.get_command_class(cmd)

    class HookedCommand(cmd_cls_orig):
        def run(self, *ar, **kwa):
            func(self, *ar, **kwa)
            res = cmd_cls_orig.run(self, *ar, **kwa)
            if func_post is not None:
                func_post(self)
            return res

    HookedCommand.__name__ = 'Hooked_%s' % (cmd_cls_orig.__name__,)
    return HookedCommand


def get_version():
    try:
        import debian.changelog
    except Exception:
        # Fallback
        import re
        data = open("debian/changelog").read(65535)
        version = re.search(r"^[^ ]+ \(([^)]+)\) .*urgency=", data).group(1)
        return version
    else:
        changelog = debian.changelog.Changelog(open('debian/changelog'))
        return changelog.version.full_version


def write_version(filename="statface_client/version_actual.py"):
    if not filename:
        return
    version = get_version()
    here = os.path.dirname(__file__)
    version_filename = os.path.join(here, filename)
    content = "__version__ = {0!r}\n".format(version)
    print(" %s writing version %r %s" % ("=" * 30, version, "=" * 30))
    with open(version_filename, 'w') as version_file:
        version_file.write(content)


def all_hooks(*args, **kwargs):
    write_version()


SETUP_KWARGS = dict(
    name='python-statface-client',
    version=get_version(),
    description='Statface REST client for humans',
    author='Mikhail Borisov, Veronika Ivannikova',
    author_email='statface-dev@yandex-team.ru',
    url='https://github.yandex-team.ru/statbox/python-statface-client',
    packages=find_packages(),
    cmdclass={  # overrides/hooks to add write_version
        'develop': mk_hook_command('develop', all_hooks),
        'build': mk_hook_command('build', all_hooks),
    },
    install_requires=[
        'requests[security]>=2.21.0',
        'urllib3>=1.24.1',
        'idna>=2.8',
        # 'certifi-yandex',
        'PyYAML',
        'six',
    ],
    package_data={
        'statface_client': ['cacert.pem'],
    },
    include_package_data=True,
)


if __name__ == '__main__':
    setup(**SETUP_KWARGS)
