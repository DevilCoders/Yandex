from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.NBSUsage import NBSUsage

class ComputeServ(YQLTask):
    sql_file_path = 'sql/compute_serv.sql'

    def requires(self):
        return NBSUsage()
