from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.SameIP import SameIP

class SamePhone(YQLTask):
    sql_file_path = 'sql/same_phone.sql'

    def requires(self):
        return SameIP()
