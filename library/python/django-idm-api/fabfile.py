from fabric.api import env, task, local

from robe import venv

env.robe.service = 'django_idm_api'
env.robe.venv.use_wheel = True
env.robe.projects = {
    'django-idm-api': {'path': './', 'use_global_site_packages': True},
}


@task()
def enter(cmd, yenv_type='development.local'):
    with venv.activate():
        local('YENV_TYPE=%s %s' % (yenv_type, cmd))


@task
def test(args=''):
    enter('py.test --ds=tests.settings %s' % args, 'development.unittest')


@task
def flake():
    enter('flake8 django_idm_api')


@task
def tox():
    local('tox --develop')
