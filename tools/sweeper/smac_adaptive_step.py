author__ = 'newo'

from numpy.random import choice
from copy import copy
from copy import deepcopy
import numpy
from sklearn.preprocessing import OneHotEncoder
from sklearn.ensemble import RandomForestRegressor
from scipy.stats import norm
import sys
import os
from argparse import ArgumentParser
import json
from sklearn.gaussian_process import GaussianProcessRegressor

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'commons'))
import fml_design_helper
# from ptpython import repl
# repl.embed(globals(), locals())


import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)

def read_from_json(json_data):
    # Parameter spaces. Example:
    # ```
    # parameter_spaces = {
    #     'name': {
    #         'values': [1,2,3,4],
    #         'type': 'GRID_INTEGER'
    #     },
    #     'name2': {
    #         'values': ['one', 'two'],
    #         'type': 'SEQUENCE'
    #     }
    # }
    # ```
    parameter_spaces = {}
    for parameter_space in json_data['options']['sweeper-options']['parameter-spaces']:
        if parameter_space['type'] in ['SEQUENCE', 'GRID_DOUBLE', 'GRID_INTEGER', 'RANGE']:
            if parameter_space['type'] == 'SEQUENCE':
                values = list(parameter_space['elements'])
            elif parameter_space['type'] == 'GRID_INTEGER':
                values = list(range(
                    parameter_space['start'],
                    parameter_space['end'] + 1,
                    parameter_space['step']
                    ))
            elif parameter_space['type'] == 'GRID_DOUBLE':
                values = list(numpy.arange(
                    parameter_space['start'],
                    parameter_space['end'] + parameter_space['step'] / 2.,
                    parameter_space['step']
                    ))
            elif parameter_space['type'] == 'RANGE':
                values = list(numpy.linspace(parameter_space['start'], parameter_space['end'], 1000))

            parameter_spaces[parameter_space['name']] = {
                'type': parameter_space['type'],
                'values': values
            }

    # Design data
    design_data = json_data["design_data"]

    # Optimization parameters
    optimization_parameters = {}
    optimization_parameters['optimization-direction'] = json_data["options"]["sweeper-adaptive-design-options"]["optimization-direction"]
    optimization_parameters['value_2_surrogate'] = json_data['value_2_surrogate']

    return parameter_spaces, design_data, optimization_parameters

def compute_mean_sigma_rf(point, rf):
    preds = list()
    for est in rf.estimators_:
        preds.append(est.predict(numpy.array(point).astype(float).reshape(1, -1)))
    m = numpy.mean(preds)
    s = numpy.std(preds) / 2
    return m, s

def compute_mean_sigma_gp(point, gp):
    m, s = gp.predict(numpy.array(point).astype(float).reshape(1, -1), True)
    return m, s

def compute_ei(m, s, current_maximum):
    u = (current_maximum - m) / (s + 0.0001)
    ei = s * (u * norm.cdf(u) + norm.pdf(u))
    return ei

def random_search_step(parameter_spaces, design_data, optimization_parameters):
    new_point = {}
    for parameter_name in parameter_spaces:
        parameter_value = choice(parameter_spaces[parameter_name]['values'])
        new_point[parameter_name] = parameter_value
    return new_point

def smac_step(parameter_spaces, design_data, optimization_parameters):
    # Handling optimization parameters
    if optimization_parameters['optimization-direction'] == 'MINIMIZATION':
        direction_sign = -1
    elif optimization_parameters['optimization-direction'] == 'MAXIMIZATION':
        direction_sign = +1
    else:
        print "ERROR, WRONG OPTIMIZATION DIRECTION"
        return None

    # Creating yet non encoded X and y
    ordered_categorical_feature_names = [x for x in parameter_spaces.keys() if parameter_spaces[x]['type'] == 'SEQUENCE']
    X_categorical = []
    for point in design_data:
        X_categorical.append([])
        for feature_name in ordered_categorical_feature_names:
            try:
                new_feature_value = parameter_spaces[feature_name]['values'].index(point['arguments'][feature_name])
            except:
                # TODO: sweeper 1.0 feature: sequence contains strings
                new_feature_value = parameter_spaces[feature_name]['values'].index(str(point['arguments'][feature_name]))
            X_categorical[-1].append(new_feature_value)
    X_categorical = numpy.array(X_categorical).astype(float)

    ordered_real_feature_names = [x for x in parameter_spaces.keys() if parameter_spaces[x]['type'] != 'SEQUENCE']
    X_real = []
    for point in design_data:
        X_real.append([])
        for feature_name in ordered_real_feature_names:
            new_feature_value = point['arguments'][feature_name]
            X_real[-1].append(new_feature_value)
    X_real = numpy.array(X_real).astype(float)

    y = []
    for point in design_data:
        y.append(direction_sign * point['values'][optimization_parameters['value_2_surrogate']])
    y = direction_sign * numpy.array(y).astype(float)

    # Encoding X
    if len(ordered_categorical_feature_names) > 0:
        encoder = OneHotEncoder(
            categorical_features='all',
            handle_unknown='ignore',
            sparse=False)
        encoder.fit(X_categorical)
        X_categorical_encoded = encoder.transform(X_categorical).astype(float)
    else:
        X_categorical_encoded = X_categorical
    X_encoded = numpy.hstack([X_categorical_encoded, X_real])

    # Training surrogate
    if optimization_parameters['method'] == 'rf':
        if 'number_of_trees' in optimization_parameters:
            n_estimators = optimization_parameters['number_of_trees']
        else:
            n_estimators = 20
        surrogate = RandomForestRegressor(n_estimators=n_estimators, n_jobs=16)
    elif optimization_parameters['method'] == 'gp':
        surrogate = GaussianProcessRegressor()
    else:
        print 'ERROR! Not such method', method
        return None
    surrogate.fit(X_encoded, y)

    # Optimize expected improvement
    points_to_try = []
    points_to_try_encoded = []
    ei_values = []
    for i in xrange(100):
        point_to_try = random_search_step(parameter_spaces, design_data, optimization_parameters)
        point_to_try_x = []
        for feature_name in ordered_categorical_feature_names:
            new_feature_value = parameter_spaces[feature_name]['values'].index(point_to_try[feature_name])
            point_to_try_x.append(new_feature_value)
        for feature_name in ordered_real_feature_names:
            new_feature_value = point_to_try[feature_name]
            point_to_try_x.append(new_feature_value)
        if len(ordered_categorical_feature_names) > 0:
            point_to_try_x_encoded = \
                    list(encoder.transform(point_to_try_x[:len(ordered_categorical_feature_names)]).ravel()) + \
                    point_to_try_x[len(ordered_categorical_feature_names):]
        else:
            point_to_try_x_encoded = point_to_try_x
        if optimization_parameters['method'] == 'rf':
            distribution_mean, distribution_sigma = compute_mean_sigma_rf(point_to_try_x_encoded, surrogate)
        elif optimization_parameters['method'] == 'gp':
            distribution_mean, distribution_sigma = compute_mean_sigma_gp(point_to_try_x_encoded, surrogate)
        else:
            print 'ERROR! Not such method', method
            return None
        ei = compute_ei(distribution_mean, distribution_sigma, max(y))
        ei_values.append(ei)
        points_to_try_encoded.append(point_to_try_x_encoded)
        points_to_try.append(point_to_try)
    new_point = points_to_try[numpy.argmax(ei_values)]
    new_point_encoded = points_to_try_encoded[numpy.argmax(ei_values)]

    if optimization_parameters['batch_size'] == 1:
        # no more batch points are needed
        return [new_point]
    else:
        # more batch points are needed
        new_optimization_parameters = deepcopy(optimization_parameters)
        new_optimization_parameters['batch_size'] -= 1

        new_design_data = deepcopy(design_data)
        new_design_data.append({'values': {optimization_parameters['value_2_surrogate']: surrogate.predict(new_point_encoded)},
                                'arguments': new_point})

        batch = smac_step(parameter_spaces, new_design_data, new_optimization_parameters)
        batch.append(new_point)
        return batch


def main():
    optparse = ArgumentParser()
    optparse.add_argument('--initial_design')
    optparse.add_argument('--result')
    optparse.add_argument('--method', default='rf')
    optparse.add_argument('--batch_size', default='1')
    opts = optparse.parse_args()

    initial_design = json.load(file(opts.initial_design, "rt"))
    parameter_spaces, design_data, optimization_parameters = read_from_json(initial_design)
    optimization_parameters['method'] = opts.method
    optimization_parameters['batch_size'] = int(opts.batch_size)

    next_point = smac_step(parameter_spaces, design_data, optimization_parameters)

    def ExtractNameForOutput(name):
        if ';' in name:
            return name.split(';')[1].strip()
        else:
            return name

    full_sweeper_output = []
    for point in next_point:
        sweeper_output = {'status': 'ok', 'next_point': copy(point)}
        for key in point:
            sweeper_output['next_point'][ExtractNameForOutput(key)] = sweeper_output['next_point'][key]
        # fallback
        for key in sweeper_output['next_point']:
            sweeper_output[key] = sweeper_output['next_point'][key]
        full_sweeper_output.append(sweeper_output)

    if optimization_parameters['batch_size'] == 1:
        full_sweeper_output = full_sweeper_output[0]

    json.dump(full_sweeper_output, file(opts.result, "wt"))


if __name__ == "__main__":
    main()
