from setuptools import setup
from setuptools import find_packages
from mkdocs_yandex import __version__

setup(
    name='mkdocs_yandex',
    version=__version__,
    packages=find_packages(),
    # They are already in Arcadia
    install_requires=[
        'jinja2',
        'mkdocs',
        'markdown',
        'python-slugify',
    ],
    entry_points={
        'mkdocs.plugins': [
            'yandex = mkdocs_yandex.plugin:MkDocsYandex',
        ],
    },
)
