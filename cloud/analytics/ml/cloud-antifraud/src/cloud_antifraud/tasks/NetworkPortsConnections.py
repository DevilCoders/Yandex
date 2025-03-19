from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.PassportRules import PassportRules
from cloud_antifraud.tasks.SamePhone import SamePhone

class NetworkPortsConnections(YQLTask):
    sql_file_path = 'sql/network_ports_connections.sql'

    def requires(self):
        return PassportRules()
