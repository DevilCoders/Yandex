import datetime
import pytest

import yaqutils.time_helpers as utime
import mstand_metric_helpers.online_metric_helpers as online_mhelp

from metrics_api import ExperimentForAPI
from metrics_api import ObservationForAPI
from experiment_pool import Experiment
from experiment_pool import Observation
from metrics_api.online import UserActionForAPI
from metrics_api.online import UserActionsForAPI

from sample_metrics.online.online_test_metrics import MetricForUsers
from sample_metrics.online.online_test_metrics import SessionsWithSplit
from sample_metrics.online.online_test_metrics import SessionsWOSplit
from sample_metrics.online.online_test_metrics import AmbiguousMetric
from sample_metrics.online.online_test_metrics import RequestMetric
from sample_metrics.online.online_test_metrics import MetricWithFiles


# here are tests mostly for online_metric_helpers.py, but they require metric instances.
# So these tests live here, together with online metric samples

# noinspection PyClassHasNoInit
class TestMetricApplication:
    dates = utime.DateRange(datetime.date(2016, 5, 2), datetime.date(2016, 5, 2))
    user = "y1337"

    control = Experiment(testid="all")

    experiment = Experiment(testid="all")
    experiment_api = ExperimentForAPI(experiment)

    observation = Observation(obs_id=None, dates=dates,
                              control=control, experiments=[experiment])
    observation_api = ObservationForAPI(observation)

    data = {"Test": u"привет", "without": "arrays", "reqid": "one_id", "type": "click", "servicetype": "web"}
    other_data = {"Tiny": "dictionary", u"hasmisspell": 1, "reqid": "other_id"}

    action = UserActionForAPI(user, 1465133772, data)
    similar_action = UserActionForAPI(user, 1465133782, data)
    other_action = UserActionForAPI(user, 1590009772, other_data)

    actions = UserActionsForAPI(user, [action, similar_action, other_action], experiment_api, observation_api)

    def test_user_metric(self):
        metric_instance = MetricForUsers()
        result = online_mhelp.apply_online_metric(metric_instance, self.actions)

        assert result == self.user

    def test_session_metric_def_split(self):
        metric_instance = SessionsWOSplit()
        result = online_mhelp.apply_online_metric(metric_instance, self.actions)

        assert len(result) == 2
        assert result[1] - result[0] > 30 * 60

    def test_session_metric_with_split(self):
        metric_instance = SessionsWithSplit()
        result = online_mhelp.apply_online_metric(metric_instance, self.actions)

        assert len(result) == 2

    def test_metric_validation(self):
        metric_instance = AmbiguousMetric()

        with pytest.raises(Exception):
            online_mhelp.validate_online_metric(metric_instance)

    def test_request_metric(self):
        metric_instance = RequestMetric()
        result = online_mhelp.apply_online_metric(metric_instance, self.actions)

        assert result == [50., 100.]

    def test_filenames(self):
        metric_instance = MetricWithFiles()
        assert hasattr(metric_instance, "list_files")
        files = metric_instance.list_files()
        assert len(files) == 1

        with pytest.raises(Exception):
            online_mhelp.validate_online_metric_files(metric_instance)
