#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import argparse
import os

import fml_wrapper.sweeper_design as sweeper_design

try:
    import nirvana.job_context as nv

    context = nv.context()
except:
    pass


PRODUCTION = True

def prepare_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--n-points')
    parser.add_argument('--seed')

    args = parser.parse_args()

    if PRODUCTION:
        args.out = {k: " ".join(v) for k, v in context.outputs.data.items()}
        args.inp = {k: " ".join(v) for k, v in context.inputs.data.items()}


        job_context_json = args.out.get('job_context_json')
        if job_context_json is not None:
            os.system('cp job_context.json {0}'.format(job_context_json))

        args.design = args.inp.get("design")
        args.next_point = args.out.get("next_points")

    else:
        args.design = "./foo/design.json"
        args.next_point = "./foo/next_points"

    # mx_ops_args.skip = parameter("skip", int, default=None)
    return args


def main():
    args = prepare_args()

    design_json = sweeper_design.get_sweeper_pass_design_from_file(args.design)

    sweeper_pass = sweeper_design.SweeperPass(design_json)

    import ghalton

    SEED = int(args.seed)
    n_dims = len(sweeper_pass.target_function_definition.input_space)
    halton = ghalton.GeneralizedHalton(n_dims, SEED)

    design_at_cube = halton.get(int(args.n_points))
    # dirty hack
    # n_at_design = len(sweeper_pass.points)


    sorted_axis = sorted(sweeper_pass.target_function_definition.input_space)
    next_points = []
    for n_at_design in xrange(int(args.n_points)):
        point_from_cube = design_at_cube[n_at_design]
        next_point = {}
        for n, axis in enumerate(sorted_axis):
            print axis.name
            st = sweeper_pass.sweep_pass_scheme.axis_name_2_sampling_type[axis.name]
            print "\t", st.data
            new_value = st.from_0_1(point_from_cube[n])
            print "\t", st, point_from_cube[n], "->", new_value
            next_point[axis.name] = new_value
        next_points.append(next_point)
    sweeper_design.produce_next_point_with_ok_status_message(args.next_point, next_points)
    pass

if __name__ == "__main__":
    main()

