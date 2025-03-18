# coding=utf-8

import json
import requests


import numpy
import scipy
import scipy.stats

__author__ = 'algorc'


def get_sweeper_pass_design_from_file(f_name):
    with open(f_name, "rt") as f_in:
        design = json.load(f_in)

    return design


def _get_sweeper_pass_design(design_id):
    session = requests.Session()
    url = "https://fml.yandex-team.ru/sweeper/pass/design?id={}".format(design_id)
    response = session.post(url, verify=False)
    if response.status_code != 200:
        return None

    resp_json = json.loads(response.text)
    return resp_json


def get_sweeper_pass_design(design_id):
    design = _get_sweeper_pass_design(design_id)
    if design is None:
        return
    else:
        return design["data"]


class Axis(object):
    def __init__(self, name, axis_type):
        self.name = name
        self.axis_type = axis_type

    @staticmethod
    def get_axis_from_pair(pair):
        axis = Axis(
            name=pair[0],
            axis_type=pair[1]
        )
        return axis

    def __repr__(self):
        return "{} : {}".format(self.name, self.axis_type)


class TargetFunctionDefinition(object):
    def __init__(self, data_json):
        self.data = data_json
        self.input_space = [Axis.get_axis_from_pair(pair) for pair in self.data['input-space'].items()]
        self.output_space = [Axis.get_axis_from_pair(pair) for pair in self.data['output-space'].items()]

    def get_function_id(self):
        return self.data['id']

    def get_description(self):
        return self.data['description']

    def __repr__(self):
        return "Function #{} [{}]".format(self.get_function_id(), self.get_description())


class SamplingType(object):
    KNOWN_TYPES = {
        "constant",
        "sequence",
        "binomial",
        "poisson",
        "discrete-uniform",
        "continuous-uniform",
        "exponential",
        "normal",
        "log-normal",
        "custom",
    }
    def __init__(self, data_json):
        self.data = data_json

        assert self.validate_type(), "Unknown axis-type '{}'".format(self.get_axis_type())

    def get_axis_type(self):
        return self.data["axis-type"]

    def validate_type(self):
        return self.get_axis_type() in self.KNOWN_TYPES

    def __repr__(self):
        # return self.get_axis_type() + " " + str(self.get_discretization())
        return self.get_axis_type() + " " + repr(self.cast_to_special())

    def cast_to_special(self):
        axis_type = self.get_axis_type()
        if axis_type == "constant":
            return ConstantST(self.data)
        elif axis_type == "sequence":
            return SequenceST(self.data)
        elif axis_type == "binomial":
            return BinomialST(self.data)
        elif axis_type == "poisson":
            return PoissonST(self.data)
        elif axis_type == "discrete-uniform":
            return DiscreteUniformST(self.data)
        elif axis_type == "continuous-uniform":
            return ContinuousUniformST(self.data)
        elif axis_type == "exponential":
            return ExponentialST(self.data)
        elif axis_type == "normal":
            return NormalST(self.data)
        elif axis_type == "log-normal":
            return LogNormalST(self.data)
        elif axis_type == "custom":
            return CustomST(self.data)
        else:
            return None

    def get_discretization(self):
        return self.data["discretization"]

    def get_sample_from_discretization_flag(self):
        return self.data["sample-from-discretization"]

    def from_0_1(self, val):
        return self.cast_to_special().from_0_1(val)

    def get_upper_bound(self):
        return self.data["end"]


class ConstantST(SamplingType):
    def __init__(self, data_json):
        super(ConstantST, self).__init__(data_json)

    def __repr__(self):
        return "ConstantST"

    def from_0_1(self, val):
        return self.data["axis-value"]

    def get_sample_from_discretization_flag(self):
        return False

    def get_discretization(self):
        return 1

    def get_upper_bound(self):
        return self.data["axis-value"]



class SequenceST(SamplingType):
    def __init__(self, data_json):
        super(SequenceST, self).__init__(data_json)

    def __repr__(self):
        return "sequenceST"

    def from_0_1(self, val):
        n_items = len(self.data["elements"])
        n = int(min(numpy.floor(val * n_items), n_items - 1))
        return self.data["elements"][n]


class BinomialST(SamplingType):
    def __init__(self, data_json):
        super(SamplingType, self).__init__()

    def __repr__(self):
        return "BinomialST"


class PoissonST(SamplingType):
    def __init__(self, data_json):
        super(PoissonST, self).__init__(data_json)

    def __repr__(self):
        return "PoissonST"

    def from_0_1(self, val):
        mu_val = float(self.data["lambda"])
        return scipy.stats.poisson.ppf(val, mu=mu_val)

    def get_upper_bound(self):
        return self.from_0_1(1.0 - self.data["quantile-eps"])



class DiscreteUniformST(SamplingType):
    def __init__(self, data_json):
        super(DiscreteUniformST, self).__init__(data_json)

    def __repr__(self):
        return "DiscreteUniformST"

    def from_0_1(self, val):
        return int(max(self.data["start"], scipy.stats.randint.ppf(val, self.data["start"], self.data["end"] + 1)))


class ContinuousUniformST(SamplingType):
    def __init__(self, data_json):
        super(ContinuousUniformST, self).__init__(data_json)

    def __repr__(self):
        return "ContinuousUniformST"

    def from_0_1(self, val):
        return scipy.stats.uniform.ppf(val, self.data["start"], self.data["end"])


class ExponentialST(SamplingType):
    def __init__(self, data_json):
        super(ExponentialST, self).__init__(data_json)

    def __repr__(self):
        return "ExponentialST"

    def from_0_1(self, val):
        scale_val = 1.0 / float(self.data["lambda"])
        return scipy.stats.expon.ppf(val, scale=scale_val)

    def get_upper_bound(self):
        return self.from_0_1(1.0 - self.data["quantile-eps"])


class NormalST(SamplingType):
    def __init__(self, data_json):
        super(SamplingType, self).__init__()

    def __repr__(self):
        return "NormalST"


class LogNormalST(SamplingType):
    def __init__(self, data_json):
        super(SamplingType, self).__init__()

    def __repr__(self):
        return "LogNormalST"


class CustomST(SamplingType):
    def __init__(self, data_json):
        super(SamplingType, self).__init__()

    def __repr__(self):
        return "CustomST"


class SweeperPassScheme(object):
    def __init__(self, data_json):
        self.data = data_json
        self.axis_name_2_sampling_type = {name : SamplingType(self.data[name]) for name in self.data}

    def __repr__(self):
        return "Sweeper pass scheme for axes:\n  * " + "\n  * ".join(
            [
                "{} [{}]".format(name, self.axis_name_2_sampling_type[name].cast_to_special())
                for name  in self.axis_name_2_sampling_type
            ]
        )


class PointStatuses(object):
    COMPLETED = "Completed"
    RUNNING = "Running"
    FAILED = "Failed"
    WAITING = "Waiting"


class Point(object):
    def __init__(self, data_json):
        self.data = data_json

    def get_status(self):
        return self.data["status"]

    def get_function_id(self):
        return self.data["function-id"]

    def get_point_id(self):
        return self.data["point-id"]


class SweeperPass(object):
    def __init__(self, data_json):
        self.data = data_json
        self.target_function_definition = TargetFunctionDefinition(self.data['target-function-definition'])
        self.sweep_pass_scheme = SweeperPassScheme(self.data['sweep-pass-scheme'])
        self.points = [Point(point_data) for point_data in self.data["points"]]

    def get_sweep_pass_id(self):
        return self.data['sweep-pass']

    def get_sweeper_task(self):
        return self.data['sweep-task']


def produce_next_point_with_error_status_message(file_name, status):
    with open(file_name, "wt") as f_out:
        data = {
            "status": status,
        }
        json.dump(data, f_out)


def produce_next_point_with_ok_status_message(file_name, next_points):
    with open(file_name, "wt") as f_out:
        data = {
            "status": "OK",
            "points": []
        }
        for point in next_points:
            data["points"].append(point)
        json.dump(data, f_out)
