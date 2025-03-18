# coding: utf-8

from fabric.api import env, task, local

from robe import venv

env.robe.service = 'cia-stuff'
env.robe.venv.pip_version = '9.0.1'
env.robe.venv.quiet_install = False
env.robe.projects = {
    'cia-stuff': {'path': './'},
}
env.robe.venv.requirements_file = 'requirements.txt'
env.robe.venv.dev_requirements_file = 'requirements_test.txt'
