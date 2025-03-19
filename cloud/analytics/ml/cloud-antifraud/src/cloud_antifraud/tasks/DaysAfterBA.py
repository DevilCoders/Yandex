from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.ComputeServ import ComputeServ

class DaysAfterBA(YQLTask):
    sql_file_path = 'sql/days_after_ba.sql'

    def requires(self):
        return ComputeServ()
