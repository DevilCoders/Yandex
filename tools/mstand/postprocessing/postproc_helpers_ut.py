import pytest

import postprocessing.postproc_helpers as pp_helpers

from postprocessing.scripts.echo import EchoPostprocessor
from postprocessing.scripts.echo import ObsEchoPostprocessor


class AmbigousPostprocessor(object):
    def __init__(self):
        pass

    def process_experiment(self, exp):
        pass

    def process_observation(self, obs):
        pass


class IncompletePostprocessor(object):
    def __init__(self):
        pass


# noinspection PyClassHasNoInit
class TestPostprocValidator:
    def test_good_postprocessors(self):
        pp_helpers.validate_processor(EchoPostprocessor())
        pp_helpers.validate_processor(ObsEchoPostprocessor())

    def test_bad_postprocessors(self):
        with pytest.raises(Exception):
            pp_helpers.validate_processor(AmbigousPostprocessor())
        with pytest.raises(Exception):
            pp_helpers.validate_processor(IncompletePostprocessor())
