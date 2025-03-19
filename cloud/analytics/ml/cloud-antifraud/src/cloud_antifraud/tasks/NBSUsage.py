from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.ServiceUsage import ServiceUsage

class NBSUsage(YQLTask):
    sql_file_path = 'sql/nbs_usage.sql'

    def requires(self):
        return ServiceUsage()
