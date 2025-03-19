"""
Fake passport recipe
"""
import os
from library.python.testing.recipe import declare_recipe, set_env
from library.python.testing.recipe.ports import get_port_range, release_port_range
from cloud.mdb.dbm.tests.recipes.passport.src.fake_passport import APP

pidfile = 'mdb-passport-recipe.pid'
ports_pidfile = 'mdb-passport-recipe-ports.pid'


def start(_):
    port = get_port_range(pid_filename=ports_pidfile)
    set_env('MDB_PASSPORT_RECIPE_PORT', str(port))

    pid = os.fork()
    if pid == 0:
        APP.run(port=port)
    else:
        with open(pidfile, 'w') as fd:
            fd.write(str(pid))


def stop(_):
    release_port_range(pid_filename=ports_pidfile)
    with open(pidfile) as fd:
        pid = int(fd.read())
        os.kill(pid, 9)


if __name__ == '__main__':
    declare_recipe(start, stop)
