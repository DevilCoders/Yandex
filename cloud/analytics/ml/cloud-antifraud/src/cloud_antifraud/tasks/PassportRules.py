from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.BadAccountsHistory import BadAccountsHistory
from cloud_antifraud.tasks.SamePhone import SamePhone

class PassportRules(YQLTask):
    sql_file_path = 'sql/passport_rules.sql'

    def requires(self):
        return [BadAccountsHistory(), SamePhone()]
