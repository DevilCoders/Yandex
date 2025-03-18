# coding=utf-8
__author__ = 'newo, algorc'
#import bar.data
#print bar.data.Data
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
        #import ipdb; ipdb.set_trace()
        new_point[parameter_name] = parameter_value
    return new_point


def smac_step(parameter_spaces, design_data, optimization_parameters):
    # Handling optimization parameters
    #import ipdb; ipdb.set_trace()
    if optimization_parameters['optimization-direction'] == 'MINIMIZATION':
        direction_sign = 1
    elif optimization_parameters['optimization-direction'] == 'MAXIMIZATION':
        direction_sign = -1
    else:
        print "ERROR, WRONG OPTIMIZATION DIRECTION"
        return None

    # Creating yet non encoded X and y
    ordered_categorical_feature_names = [x for x in parameter_spaces.keys() if parameter_spaces[x]['type'] == 'sequence']
    X_categorical = []
    for point in design_data:
        X_categorical.append([])
        for feature_name in ordered_categorical_feature_names:
            try:
                new_feature_value = parameter_spaces[feature_name]['values'].index(point['inputs'][feature_name])
            except:
                # TODO: sweeper 1.0 feature: sequence contains strings
                new_feature_value = parameter_spaces[feature_name]['values'].index(str(point['inputs'][feature_name]))
            X_categorical[-1].append(new_feature_value)
    X_categorical = numpy.array(X_categorical).astype(float)

    ordered_real_feature_names = [x for x in parameter_spaces.keys() if parameter_spaces[x]['type'] != 'sequence']

    X_real = []
    for point in design_data:
        X_real.append([])
        for feature_name in ordered_real_feature_names:
            new_feature_value = point['inputs'][feature_name]
            X_real[-1].append(new_feature_value)
    X_real = numpy.array(X_real).astype(float)

    y = []
    for point in design_data:
        y.append(direction_sign * point['outputs'][optimization_parameters['value_2_surrogate']])
    y = numpy.array(y).astype(float) # now we a searching for minimum

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
    method = optimization_parameters['method']
    if method == 'rf':
        if 'number_of_trees' in optimization_parameters:
            n_estimators = optimization_parameters['number_of_trees']
        else:
            n_estimators = 20
        surrogate = RandomForestRegressor(n_estimators=n_estimators, n_jobs=16)
    elif method == 'gp':
        surrogate = GaussianProcessRegressor()
    else:
        print 'ERROR! Not such method', method
        return None
    surrogate.fit(X_encoded, y)

    # Optimize expected improvement
    points_to_try = []
    points_to_try_encoded = []
    ei_values = []
    for i in xrange(1000): # TODO: SWEEPER-9
        point_to_try = random_search_step(parameter_spaces, design_data, optimization_parameters)
        #import ipdb; ipdb.set_trace()
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
        if method == 'rf':
            distribution_mean, distribution_sigma = compute_mean_sigma_rf(point_to_try_x_encoded, surrogate)
        elif method == 'gp':
            distribution_mean, distribution_sigma = compute_mean_sigma_gp(point_to_try_x_encoded, surrogate)
        else:
            print 'ERROR! Not such method', method
            return None
        ei = compute_ei(distribution_mean, distribution_sigma, max(y))
        ei_values.append(ei)
        points_to_try_encoded.append(point_to_try_x_encoded)
        points_to_try.append(point_to_try)
    print "Maximum of EI: ", ei_values[numpy.argmax(ei_values)]
    print "Mean of EI: ", numpy.mean(ei_values)
    print "Std of EI: ", numpy.std(ei_values)
    print ""
    #import ipdb; ipdb.set_trace()
    pass
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
        numpy_new_point_encoded = numpy.array(new_point_encoded).reshape(1, len(new_point_encoded))
        new_value = surrogate.predict(numpy_new_point_encoded)
        new_design_data.append(
            {
                'outputs': {
                    optimization_parameters['value_2_surrogate']: new_value},
                'inputs': new_point
            }
        )

        batch = smac_step(parameter_spaces, new_design_data, new_optimization_parameters)
        batch.append(new_point)
        return batch
