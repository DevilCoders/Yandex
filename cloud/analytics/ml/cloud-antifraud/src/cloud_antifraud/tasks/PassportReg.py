from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.AccountNameDist import AccountNameDist

class PassportReg(YQLTask):
    sql_file_path = 'sql/passport_reg.sql'

    def requires(self):
        return AccountNameDist()
