from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.ComputeFeatures import ComputeFeatures

class SdnFeatures(YQLTask):
    sql_file_path = 'sql/sdn_features.sql'

    def requires(self):
        return ComputeFeatures()
