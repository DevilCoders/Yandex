from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.DaysAfterPUID import DaysAfterPUID

class AccountNameDist(YQLTask):
    sql_file_path = 'sql/account_name_dist.sql'

    def requires(self):
        return DaysAfterPUID()
