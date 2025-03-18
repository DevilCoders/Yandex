#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import argparse
import os
import numpy

import fml_wrapper.sweeper_design as sweeper_design
import adaptive_design.smac_adaptive_step as smac_adaptive_step


try:
    import nirvana.job_context as nv

    context = nv.context()
except:
    pass


PRODUCTION = True


def parameter(name, convert, required=False, default=None, transform=None):
    value = context.parameters.get(name)
    value = convert(value) if value is not None and value != "" else default
    if required and value is None:
        raise ValueError("required parameter %s is missing" % name)
    return transform(value) if value is not None and transform is not None else value


get_parameter_form_options = parameter  # make an alias


def prepare_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--n-points')
    parser.add_argument('--seed')
    parser.add_argument('--debug', action="store_true")

    args = parser.parse_args()

    if args.debug:
        global PRODUCTION
        PRODUCTION = False


    if PRODUCTION:
        args.out = {k: " ".join(v) for k, v in context.outputs.data.items()}
        args.inp = {k: " ".join(v) for k, v in context.inputs.data.items()}


        job_context_json = args.out.get('job_context_json')
        if job_context_json is not None:
            os.system('cp job_context.json {0}'.format(job_context_json))

        args.design = args.inp.get("design")
        args.next_point = args.out.get("next_points")

        args.target_name = get_parameter_form_options("target", str, default=None)
        args.optimization_direction = get_parameter_form_options("optimization-direction", str, default=None)
        args.range_discretization = get_parameter_form_options("range-discretization", int, default=None)
        args.batch_size = get_parameter_form_options("batch-size", int, default=None)
        args.surrogate_type = get_parameter_form_options("surrogate-type", str, default=None)

    else:
        case = 2
        if case == 1:
            args.design = "./foo/design.json"
            args.next_point = "./foo/next_points_178"

            args.target_name = "r2_main" #"train_avg_location_inverted_f1"
            args.optimization_direction = "MAXIMIZATION" #"MINIMIZATION"
            args.range_discretization = 1000
            args.batch_size = 5
            args.surrogate_type = "rf"
        elif case == 2:
            pref = "test_data/"
            args.design = pref + "SWEEPER-7_design"
            args.next_point = pref + "SWEEPER-7_next_point"

            args.target_name = "[train] f1" #"train_avg_location_inverted_f1"
            args.optimization_direction = "MAXIMIZATION" #"MINIMIZATION"
            args.range_discretization = 1000
            args.batch_size = 5
            args.surrogate_type = "rf"

    return args


def get_parameter_spaces(sweeper_pass, RANGE_DISCRETIZATION):
    axis_name_2_values = {}
    axis_name_2_values_list = []
    for axis_name, sampling_type in sweeper_pass.sweep_pass_scheme.axis_name_2_sampling_type.iteritems():
        special_sampling_type = sampling_type.cast_to_special()
        print special_sampling_type.data
        if special_sampling_type.get_axis_type() == "sequence":
            values = special_sampling_type.data["elements"]
        else:
            special_sampling_type = sampling_type.cast_to_special()
            is_sample_from_discretization = special_sampling_type.get_sample_from_discretization_flag()
            if is_sample_from_discretization:
                discretization = special_sampling_type.get_discretization()
            else:
                discretization = RANGE_DISCRETIZATION
            print RANGE_DISCRETIZATION
            values = [sampling_type.from_0_1(v) for v in numpy.linspace(0, 1, discretization)]
            values[-1] = special_sampling_type.get_upper_bound()  # to avoid numericall noise

        axis_name_2_values[axis_name] = values
        axis_name_2_values_list.append(values)

    parameter_spaces = {}

    for axis_name in axis_name_2_values:
        parameter_spaces[axis_name] = {
            "type": sweeper_pass.sweep_pass_scheme.axis_name_2_sampling_type[axis_name].cast_to_special().get_axis_type(),
            "values": axis_name_2_values[axis_name]
        }

    return parameter_spaces


def run(args):
    design_json = sweeper_design.get_sweeper_pass_design_from_file(args.design)

    sweeper_pass = sweeper_design.SweeperPass(design_json)

    parameter_spaces = get_parameter_spaces(sweeper_pass, args.range_discretization)
    optimization_parameters = {
        "optimization-direction": args.optimization_direction,
        "value_2_surrogate": args.target_name,
        "method": args.surrogate_type,
        "batch_size": args.batch_size,
    }

    design_data = [point.data for point in sweeper_pass.points if point.get_status() == "Completed"]

    next_points = smac_adaptive_step.smac_step(parameter_spaces, design_data, optimization_parameters)

    sweeper_design.produce_next_point_with_ok_status_message(args.next_point, next_points)

    pass


def main():
    args = prepare_args()

    run(args)


if __name__ == "__main__":
    main()

