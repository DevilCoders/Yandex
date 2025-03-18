# coding: utf-8
import os

from fabric.api import env, task, local

from robe import venv

env.robe.service = 'django-ids'
env.robe.venv.pip_version = '9.0.1'
env.robe.venv.quiet_install = False
env.robe.projects = {
    'django-ids': {'path': './'},
}
