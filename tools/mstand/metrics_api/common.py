import uuid

from experiment_pool import Experiment  # noqa
from experiment_pool import Observation  # noqa
from user_plugins import PluginKey  # noqa


# NOTICE: Classes below are used directly in user metrics.
# Interface/fields should NOT be changed/renamed/etc. without 'mstand community' approval.


class ObservationForAPI(object):
    def __init__(self, observation, unique_str=None):
        """
        :type observation: Observation
        :type unique_str: str | None
        """
        if observation.dates:
            self.date_from = observation.dates.start
            self.date_to = observation.dates.end
            self.timestamp_from = observation.dates.start_timestamp
            self.timestamp_to = observation.dates.end_timestamp
        else:
            self.date_from = None
            self.date_to = None
            self.timestamp_from = None
            self.timestamp_to = None

        self.extra_data = observation.extra_data

        if unique_str is not None:
            self.unique_str = str(unique_str)
        else:
            self.unique_str = uuid.uuid4()

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "ObsForAPI([{}:{}])".format(self.date_from, self.date_to)


class ExperimentForAPI(object):
    def __init__(self, experiment, unique_str=None):
        """
        :type experiment: Experiment
        :type unique_str: str | None
        """
        self.testid = experiment.testid
        self.serpset_id = experiment.serpset_id
        self.extra_data = experiment.extra_data

        if unique_str is not None:
            self.unique_str = str(unique_str)
        else:
            self.unique_str = uuid.uuid4()

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "ExpForAPI(t={}, s={})".format(self.testid, self.serpset_id)


class MetricKeyForAPI(object):
    def __init__(self, metric_key):
        """
        :type metric_key: PluginKey
        """
        self.name = metric_key.name
        self.kwargs_name = metric_key.kwargs_name
        self.pretty_name = metric_key.pretty_name()

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "<{}>".format(self.pretty_name)
