from reports import CriteriaPair  # noqa
from user_plugins import PluginKey  # noqa


class CorrCalcContext(object):
    def __init__(self, criteria_pair, main_metric_key, min_pvalue):
        """
        :type criteria_pair: CriteriaPair
        :type main_metric_key: PluginKey
        :type min_pvalue: float
        """
        self.criteria_pair = criteria_pair
        self.main_metric_key = main_metric_key
        self.min_pvalue = min_pvalue


class CorrOutContext(object):
    def __init__(self, output_file, output_tsv, save_to_dir):
        """
        :type output_file: str | None
        :type output_tsv: str | None
        :type save_to_dir: str | None
        """
        self.save_to_dir = save_to_dir
        self.output_file = output_file
        self.output_tsv = output_tsv
