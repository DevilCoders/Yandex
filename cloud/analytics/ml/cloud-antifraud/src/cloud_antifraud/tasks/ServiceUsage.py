from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.SdnFeatures import SdnFeatures

class ServiceUsage(YQLTask):
    sql_file_path = 'sql/service_usage.sql'

    def requires(self):
        return SdnFeatures()
