"""
DBM recipe
"""
import os
import traceback
from library.python.testing.recipe import declare_recipe, set_env
import yatest.common.network
import yaml
from flask import make_response
from werkzeug.exceptions import HTTPException


pm = yatest.common.network.PortManager()


pidfile = 'mdb-dbm-recipe.pid'


def _base_config():
    with open(yatest.common.source_path('cloud/mdb/dbm/app.yaml')) as fd:
        return yaml.safe_load(fd)


def _fill_dbm_config(dbm_metrics_port):
    config = _base_config()
    config['BLACKBOX_URL'] = f'http://localhost:{os.environ["MDB_PASSPORT_RECIPE_PORT"]}/blackbox'
    config['DEPLOY_API_V2_URL'] = f'http://localhost:{os.environ["MDB_DEPLOY_API_RECIPE_PORT"]}/'
    config['DB_CONN'] = (
        f'port={os.environ["DBM_POSTGRESQL_RECIPE_PORT"]} dbname=dbm user=dbm '
        'password=dbm_pass connect_timeout=1 sslmode=disable'
    )
    config['DB_HOSTS'] = os.environ["DBM_POSTGRESQL_RECIPE_HOST"]

    prometheus_multiproc_dir = yatest.common.work_path('dbm-prometheus-multiproc')
    os.mkdir(prometheus_multiproc_dir)
    os.environ['prometheus_multiproc_dir'] = prometheus_multiproc_dir
    os.environ['PROMETHEUS_MULTIPROC_DIR'] = prometheus_multiproc_dir
    config['METRICS'] = {
        'port': dbm_metrics_port,
    }
    config['LOGCONFIG'] = {
        'handlers': {
            'tskv': {
                'class': 'logging.FileHandler',
                'filename': yatest.common.output_path('dbm.log'),
            }
        }
    }

    return config


def start(_):
    dbm_port = pm.get_port()
    dbm_metrics_port = pm.get_port()
    dbm_config = yatest.common.output_path('dbm-config.yaml')
    with open(dbm_config, 'w') as fd:
        yaml.dump(_fill_dbm_config(dbm_metrics_port), fd)

    os.environ['DBM_CONFIG'] = dbm_config
    set_env('MDB_DBM_PORT', str(dbm_port))
    set_env('MDB_DBM_METRICS_PORT', str(dbm_metrics_port))
    pid = os.fork()
    if pid == 0:
        # use function level import,
        # cause DBM load config during its import
        from cloud.mdb.dbm.internal.run import APP

        @APP.errorhandler(Exception)
        def handle_exception(e):
            if isinstance(e, HTTPException):
                return e
            response = make_response(traceback.format_exc())
            response.headers['Content-Type'] = 'text/plain'
            return response

        APP.run(port=dbm_port)
    else:
        with open(pidfile, 'w') as fd:
            fd.write(str(pid))


def stop(_):
    with open(pidfile) as fd:
        pid = int(fd.read())
        os.kill(pid, 9)


if __name__ == '__main__':
    declare_recipe(start, stop)
