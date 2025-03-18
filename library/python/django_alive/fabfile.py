from fabric.api import env, task, local

from robe.bundle.library import *


env.robe.service = 'django_alive'


@task()
def enter(cmd, yenv_type='development.local'):
    with venv.enter():
        local('YENV_TYPE=%s %s' % (yenv_type, cmd))

@task
def test(args=''):
    enter('py.test %s' % args, 'development.unittest')
