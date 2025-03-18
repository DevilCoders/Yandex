import yaqutils.time_helpers as utime
from experiment_pool import Experiment
from experiment_pool import Observation
from experiment_pool import Pool


def build_mc_pool(serpsets):
    null_dates = utime.DateRange(start=None, end=None)

    control_serpset_id = serpsets.pop()
    control = Experiment(testid=None, serpset_id=control_serpset_id)
    experiments = []
    for exp_serpset_id in serpsets:
        exp = Experiment(testid=None, serpset_id=exp_serpset_id)
        experiments.append(exp)

    observation = Observation(obs_id=None, dates=null_dates, control=control, experiments=experiments)
    return Pool([observation])
