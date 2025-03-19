from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.DaysAfterBA import DaysAfterBA

class DaysAfterPUID(YQLTask):
    sql_file_path = 'sql/days_after_puid.sql'

    def requires(self):
        return DaysAfterBA()
