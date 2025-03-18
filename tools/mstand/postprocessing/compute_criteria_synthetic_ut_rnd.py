import logging
import pytest
import random

import numpy.random
import scipy.stats

from postprocessing.compute_criteria_test_lib import (
    calc_criteria_result_on_observation,
    make_test_observation,
)


@pytest.fixture(autouse=True)
def random_state(request):
    seed = request.config.getoption("--seed")
    logging.info('random seed: %d', seed)
    random.seed(seed)
    numpy.random.seed(seed)


def generate_normal_data(data_size, good):
    data_x = []
    # WARNING: data_y will be ignored in online tests (test_criteria_synthetic_values_*)
    data_y = []

    for key in range(data_size):
        signal_x = random.normalvariate(0.0, 1.0)
        signal_y = random.normalvariate(0.0, 1.0)

        if good:
            values_x = [signal_x]
            values_y = [signal_y]
        else:
            values_x = [signal_x, signal_x / 2.0, signal_x * 2.0]
            values_y = [signal_y, signal_y / 2.0, signal_y * 2.0]

        data_x.append(values_x)
        data_y.append(values_y)

    return data_x, data_y


def generate_cauchy_data(data_size):
    cauchy_dist = scipy.stats.cauchy()
    data_x = cauchy_dist.rvs(data_size)
    data_y = cauchy_dist.rvs(data_size)

    return data_x, data_y


def write_value(fd, is_keyed, values, rkey):
    """
    :type fd:
    :type is_keyed: bool
    :type values: list[float]
    :type rkey: str | int
    :return:
    """
    if isinstance(values, list):
        str_vals = "\t".join([str(val) for val in values])
    else:
        str_vals = str(values)

    if is_keyed:
        fd.write("{}\t{}\n".format(rkey, str_vals))
    else:
        fd.write("{}\n".format(str_vals))


def write_synthetic_data_to_tsv(data_x, data_y, control_file, exp_file, keyed):
    logging.info("control file: %s", control_file)
    logging.info("exp_file: %s", exp_file)

    with open(control_file, "w") as control_fd:
        with open(exp_file or "/dev/null", "w") as exp_fd:
            for key, (values_x, values_y) in enumerate(zip(data_x, data_y)):
                write_value(control_fd, keyed, values_x, key)
                write_value(exp_fd, keyed, values_y, key)


def validate_synth_summary(synth_results, good):
    """
    :type synth_results: list[SyntheticSymmary]
    :type good: bool
    :rtype:
    """
    assert synth_results
    for index, synth_sum in enumerate(synth_results):
        logging.info("synth result %d info", index)
        failed_col_info = []
        """:type failed_col_info: list[dict]"""
        for col_info in synth_sum.coloring_info.colorings:
            logging.info("--> coloring info: %s", col_info)
            if good != col_info["conf_status"]:
                failed_col_info.append(col_info)
        if good and failed_col_info:
            for col_info in failed_col_info:
                logging.error("Synthetic criteria failed for level %s", col_info)
            for col_info in failed_col_info:
                if col_info["threshold"] == 0.01:
                    raise Exception("Synthetic criteria failed for 0.01, see errors above for details.")
            pass
            # raise Exception("Synthetic criteria failed for some level, see errors above for details.")


# noinspection PyClassHasNoInit
class TestComputeSyntheticCriteria:
    data_size = 2000

    def test_criteria_synthetic_values_good_normal(self, tmpdir, data_path, project_path):
        control_file = str(tmpdir.join("control_synth_good.tsv"))

        good = True
        keyed = False
        data_x, data_y = generate_normal_data(data_size=self.data_size, good=good)
        write_synthetic_data_to_tsv(data_x, data_y, control_file, exp_file=None, keyed=keyed)
        obs = make_test_observation(control_file, exp_file=None, keyed=keyed, synthetic=True, data_path=data_path)

        _, synth_res = calc_criteria_result_on_observation(obs, base_dir=project_path, synthetic=True)
        validate_synth_summary(synth_res, good=good)

    def test_criteria_synthetic_values_bad_dependent(self, tmpdir, data_path, project_path):
        control_file = str(tmpdir.join("control_synth_bad_dependent.tsv"))

        good = False
        keyed = False
        data_x, data_y = generate_normal_data(data_size=self.data_size, good=good)
        write_synthetic_data_to_tsv(data_x, data_y, control_file, exp_file=None, keyed=keyed)
        obs = make_test_observation(control_file, exp_file=None, keyed=keyed, synthetic=True, data_path=data_path)

        _, synth_res = calc_criteria_result_on_observation(obs, base_dir=project_path, synthetic=True, flatten_mode=True)
        validate_synth_summary(synth_res, good=good)

    def test_criteria_synthetic_values_bad_cauchy(self, tmpdir, data_path, project_path):
        control_file = str(tmpdir.join("control_synth_bad_cauchy.tsv"))

        good = False
        keyed = False
        data_x, data_y = generate_cauchy_data(data_size=self.data_size)
        write_synthetic_data_to_tsv(data_x, data_y, control_file, exp_file=None, keyed=keyed)
        obs = make_test_observation(control_file, None, keyed=keyed, synthetic=True, data_path=data_path)

        _, synth_res = calc_criteria_result_on_observation(obs, base_dir=project_path, synthetic=True)
        validate_synth_summary(synth_res, good=good)

    def test_criteria_synthetic_key_values_good(self, tmpdir, data_path, project_path):
        control_file = str(tmpdir.join("keyed_control_synth.tsv"))
        exp_file = str(tmpdir.join("keyed_exp_synth.tsv"))

        good = True
        keyed = True
        data_x, data_y = generate_normal_data(data_size=self.data_size, good=good)
        write_synthetic_data_to_tsv(data_x, data_y, control_file, exp_file, keyed=keyed)

        obs = make_test_observation(control_file, exp_file, keyed=keyed, synthetic=True, data_path=data_path)
        _, synth_res = calc_criteria_result_on_observation(obs, base_dir=project_path, synthetic=True)
        validate_synth_summary(synth_res, good=good)

    def test_criteria_synthetic_key_values_bad_dependent(self, tmpdir, data_path, project_path):
        control_file = str(tmpdir.join("keyed_control_synth_dependent.tsv"))
        exp_file = str(tmpdir.join("keyed_exp_synth_dependent.tsv"))

        good = False
        keyed = True
        data_x, data_y = generate_normal_data(data_size=self.data_size, good=good)
        write_synthetic_data_to_tsv(data_x, data_y, control_file, exp_file, keyed=keyed)

        obs = make_test_observation(control_file, exp_file, keyed=keyed, synthetic=True, data_path=data_path)

        _, synth_res = calc_criteria_result_on_observation(obs, base_dir=project_path, synthetic=True, flatten_mode=True)
        validate_synth_summary(synth_res, good=good)

    def test_criteria_synthetic_key_values_bad_cauchy(self, tmpdir, data_path, project_path):
        control_file = str(tmpdir.join("keyed_control_synth_cauchy.tsv"))
        exp_file = str(tmpdir.join("keyed_exp_synth_cauchy.tsv"))

        good = False
        keyed = True
        data_x, data_y = generate_cauchy_data(data_size=self.data_size)
        write_synthetic_data_to_tsv(data_x, data_y, control_file, exp_file, keyed=keyed)

        obs = make_test_observation(control_file, exp_file, keyed=keyed, synthetic=True, data_path=data_path)
        _, synth_res = calc_criteria_result_on_observation(obs, base_dir=project_path, synthetic=True)
        validate_synth_summary(synth_res, good=good)
