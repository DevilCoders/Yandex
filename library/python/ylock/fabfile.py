from fabric.api import env, task, local

from robe import venv

env.robe.projects = {
    'ylock': {}
}


enter = venv.enter

@task
def test(args=''):
    enter('py.test -vv %s' % args)
