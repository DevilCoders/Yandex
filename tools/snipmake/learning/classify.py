#!/usr/bin/env python
# -*- coding: utf8 -*-
from sys import argv
from optparse import OptionParser
from operator import itemgetter
from math import sqrt, acos, pi
import re
import os

from common import readConfig
from candidate_parser import SnippetsData
from learning_tools import randomFileName

class LinearClassifier:
    def __init__(self, coefs):
        self.coefs = coefs
        
    # output class: [-1, 1]
    def Classify(self, sample):
        return 1 if self.CalcWeight(sample) > 0 else -1
        
    def CalcWeight(self, sample):
        value = 0
        for index in range(len(self.coefs)):
            value += self.coefs[index]*sample.features[index + 1]
        return value

# *** Calculate angle between hyperplanes ***

def calculate_angle_between_hyperplanes(inc_model1_file, inc_model2_file):
    coefs1 = readIncFile(inc_model1_file)
    coefs2 = readIncFile(inc_model2_file)
    if len(coefs1) != len(coefs2):
        raise Exception, "Model dimensions should be same"

    value = 0
    model1_norm = 0
    model2_norm = 0
    
    for id in range(len(coefs1)):
        model1_norm += coefs1[id]**2
        model2_norm += coefs2[id]**2
        value += coefs1[id]*coefs2[id]
    cs =  value / (sqrt(model1_norm)*sqrt(model2_norm))
    if cs > 1 and cs < 1 + 1E-5:
        cs = 1
    elif cs < -1 and cs > -1 - 1E-5:
        cs = -1
    return 180*acos(cs)/pi
        
# *** Calculate the first best sample metric ***

def get_max_target_samples_set(target_sample_tuples):
    max_target = max(target_sample_tuples)[0]
    max_sample_set = set()
    for target_sample in target_sample_tuples:
        if target_sample[0] == max_target:
            max_sample_set.add(target_sample[1])
    return max_sample_set
    
def calculate_first_sample_metric(data, coefs):
    if len(data.objects) == 0:
        raise "Test set should contain one sample at least"
        
    classifier = LinearClassifier(coefs)
        
    task_target_sample_tuples = []
    current_id = data.objects[0].id    
    task_count = 1
    error_count = 0
    
    for sample_index in range(len(data.objects)):
        sample = data.objects[sample_index]
        
        if current_id == sample.id:
            task_target_sample_tuples.append((sample.target, sample))
            
        if (current_id != sample.id) or sample_index + 1 == len(data.objects):
            max_target_samples_set = get_max_target_samples_set(task_target_sample_tuples)
            new_task_target_sample_tuples = []
            for target_sample in task_target_sample_tuples:
                new_task_target_sample_tuples.append((classifier.CalcWeight(target_sample[1]), target_sample[1]))
            new_max_target_samples_set = get_max_target_samples_set(new_task_target_sample_tuples)
            
            if len(max_target_samples_set & new_max_target_samples_set) == 0:
                error_count += 1
            
            if sample_index + 1 != len(data.objects):
                current_id = sample.id
                task_target_sample_tuples = [(sample.target, sample)]
                task_count += 1
    return error_count, task_count

def FirstGroupError(data, inverted = None):
    task_count = 0
    error_count = 0
    for id, sample_tuples in data.iteritems():
        max_target = max([sample[0] for sample in sample_tuples])
        if inverted == "target" or inverted == "both":
            max_target = min([sample[0] for sample in sample_tuples])
        max_predicted = max([sample[1] for sample in sample_tuples])
        if inverted == "predicted" or inverted == "both":
            max_predicted = min([sample[1] for sample in sample_tuples])
        good = False
        for sample in sample_tuples:
            if sample[0] == max_target and sample[1] == max_predicted:
                good = True
        if not good:
            error_count += 1
        task_count += 1
    return error_count, task_count

# *** Calculate pair ranking error ***
    
def linear_classify_data_samples(data, coefs):
    classifier = LinearClassifier(coefs)
    error_count = 0
    for sample in data.objects:
        predicted_label = classifier.Classify(sample)
        if predicted_label != sample.target:
            error_count += 1
    return error_count, len(data.objects)

def PairClassificationError(data, inverted = None):
    error_count = 0
    task_count = 0
    invertmult = -1.0 if inverted == "predicted" or inverted == "target" else 1.0
    for id, sample_tuples in data.iteritems():
        for i in range(0, len(sample_tuples)):
            for j in range(i+1, len(sample_tuples)):                
                if sample_tuples[i][0] != sample_tuples[j][0]:
                    d_target = sample_tuples[i][0] - sample_tuples[j][0]
                    d_predicted = sample_tuples[i][1] - sample_tuples[j][1]
                    if d_target * d_predicted * invertmult <= 0:
                        error_count += 1
                    task_count += 1
    return error_count, task_count

def readIncFile(filename):
    coefs = []
    try:
        with open(filename, "r") as incfile:
            for line in incfile:
                line = re.sub("\/\*(.*)\*\/", "", line.strip()).strip(",").strip()
                coefs.append(float(line))
    except Exception as message:
        raise message
    return coefs

def fillData(data, modelFile, modelType, learner):
    if modelType in ['LibLinear', 'SVMLight']:
        model_coefs = readIncFile(modelFile)
        if model_coefs:
            print 'model for', len(model_coefs), 'factors loaded'
    elif modelType is 'MatrixNET':
        if not modelFile.endswith(".bin"):
            raise Exception("Please specify .bin file as a model file.")        
        tempfilename = randomFileName()
        resfilename = randomFileName()
        data.ToTreenet(tempfilename)
        learner.Classify(tempfilename, modelFile, resfilename)        
        res = open(resfilename, "r")
        results = res.readlines()
        res.close()
        os.remove(resfilename)
        os.remove(tempfilename)
    else:
        raise Exception, "Test mode doesn't support " + modelType + " yet."
        
    collection = {}
    index = 0
    for candidate in data.GetObjects():    
        target = candidate.target
        if modelType in ['LibLinear','SVMLight']:
            if len(model_coefs) != len(candidate.features):
                raise ValueError, "Different model and data dimensions"
            predicted = sum([ model_coefs[i-1]*candidate.features[i] for i in candidate.features.iterkeys() ])            
        elif modelType is 'MatrixNET':
            predicted = float(results[index].strip().split("\t")[-1]) # Take last field of .matrixnet file
        else:
            predicted = float(candidate.target)
        keyid = candidate.id # (candidate.id, candidate.assessor)
        if keyid not in collection:
            collection[keyid] = []
        collection[keyid].append((target, predicted))
        index += 1
    return collection

if __name__=="__main__":
    try:
        print 'Angle between hyperplanes is {0} degree'.format(calculate_angle_between_hyperplanes(argv[1], argv[2]))
    except BaseException as message:
        print '''usage: classify.py .inc1 .inc2
calculates angel between hyperplanes, .inc1 and .inc2 should be linear
try 'learning.py test' to test how formula works on data

Error:''', message