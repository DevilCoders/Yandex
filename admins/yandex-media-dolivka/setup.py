"""setup"""
from setuptools import setup, find_packages

setup(
    name="media_dolivka",
    version="0",
    author="Artem Khokhlov",
    author_email="media-admin@yandex-team.ru",
    description=("dolivka api"),
    license="GPL",
    url="https://github.yandex-team.ru/chrono/buhlo",
    packages=find_packages(),
    scripts=[
        'dolivka_api.py',
        ],
    install_requires=[
        'flask-socketio',
        'eventlet==0.21.0',
        'pyyaml',
        'pyinotify',
        'itsdangerous',
        'jinja2',
        'markupsafe',
        'dotmap',
        'kazoo',
        'requests',
        'werkzeug',
    ],
)
