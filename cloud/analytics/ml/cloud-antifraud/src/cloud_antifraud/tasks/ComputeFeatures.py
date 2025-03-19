from cloud_antifraud.tasks.YQLTask import YQLTask
from cloud_antifraud.tasks.All1HAfterTrial import All1HAfterTrial

class ComputeFeatures(YQLTask):
    sql_file_path = 'sql/compute_features.sql'

    def requires(self):
        return All1HAfterTrial()

