from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.PassportReg import PassportReg

class SameIP(YQLTask):
    sql_file_path = 'sql/same_ip.sql'

    def requires(self):
        return PassportReg()
