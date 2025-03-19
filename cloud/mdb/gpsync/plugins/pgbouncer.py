from cloud.mdb.gpsync import plugin
from cloud.mdb.gpsync import helpers


class PgbouncerPlugin(plugin.PostgresPlugin):
    def before_populate_recovery_conf(self, master_host):
        cmd = 'sudo sed -i /etc/pgbouncer/pgbouncer.ini -e "/^* = /s/host=.*$/host=' + master_host + ' port=6432/"'
        helpers.subprocess_popen(cmd)

    def before_promote(self):
        cmd = 'sudo sed -i /etc/pgbouncer/pgbouncer.ini -e "/^* = /s/host=.*$/host=localhost/"'
        helpers.subprocess_popen(cmd)
