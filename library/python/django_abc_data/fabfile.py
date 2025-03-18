from fabric.api import env, task, local

from robe import venv

env.robe.service = 'django_abc_data'
env.robe.venv.use_wheel = True
env.robe.projects = {
    'django-abc-data': {'path': './'},
}


@task()
def enter(cmd, yenv_type='development.local'):
    with venv.activate():
        local('YENV_TYPE=%s %s' % (yenv_type, cmd))


@task
def test(args=''):
    enter('py.test --ds=django_abc_data.tests.settings %s' % args, 'development.unittest')
