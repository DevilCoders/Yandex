"""
Fake deploy-api implementation
"""
import os
from flask import Flask, jsonify
from library.python.testing.recipe import declare_recipe, set_env
from library.python.testing.recipe.ports import get_port_range, release_port_range

APP = Flask('fake_deploy_api')


@APP.route('/ping')
def ping():
    """
    Return ok on any request
    """
    return 'pong\n'


@APP.route('/v1/shipments', methods=['POST'])
def shipments(**_):
    """
    Return test jid on any request
    """
    return jsonify({'id': 'test-jid'})


pidfile = 'mdb-deploy-api-recipe.pid'


def start(_):
    port = get_port_range()
    set_env('MDB_DEPLOY_API_RECIPE_PORT', str(port))

    pid = os.fork()
    if pid == 0:
        APP.run(port=port)
    else:
        with open(pidfile, 'w') as fd:
            print(f'after start deploy api {pid=}')
            fd.write(str(pid))


def stop(_):
    release_port_range()
    with open(pidfile) as fd:
        pid = int(fd.read())
        print(f'before stop deploy api {pid=}')
        os.kill(pid, 9)


if __name__ == '__main__':
    declare_recipe(start, stop)
