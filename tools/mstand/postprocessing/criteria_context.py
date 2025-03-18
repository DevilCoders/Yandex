from user_plugins import PluginKey  # noqa
from experiment_pool import Observation  # noqa
from experiment_pool import MetricDataType

from postprocessing import CriteriaReadMode


class CriteriaContext(object):
    def __init__(self, criteria, criteria_key, observation, base_dir, data_type, read_mode, synth_count=None):
        """
        :type criteria: callable
        :type criteria_key: PluginKey
        :type observation: Observation
        :type base_dir: str
        :type data_type: str
        :type read_mode: str
        :type synth_count: int | None
        """

        if data_type not in MetricDataType.ALL:
            raise Exception("Invalid data_type in CriteriaContext: {}".format(data_type))

        if read_mode not in CriteriaReadMode.ALL:
            raise Exception("Invalid read_mode in CriteriaContext: {}".format(read_mode))

        self.criteria = criteria
        self.criteria_key = criteria_key
        self.observation = observation
        self.base_dir = base_dir
        self.data_type = data_type
        self.read_mode = read_mode
        self.synth_count = synth_count
