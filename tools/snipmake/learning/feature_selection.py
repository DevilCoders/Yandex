#!/usr/bin/env python
# -*- coding: utf8 -*-
import random
import os

from sys import argv
from optparse import OptionParser
from operator import itemgetter
from math import sqrt, ceil, exp, acos, pi, log as lg
import re
import uuid
from common import *

from candidate_parser import SnippetsData
from candidate_tools import mergeCandidates
from common import readConfig
from learning import FactorsGetter, getFilter, LearningDataFromXmlPreparer, getLinearFormula

from learning_tools import LearningData, LearningObject, SVMLightLearner, LibSVMLearner, MatrixNETLearner, \
                           SVMModel, LibSVMModel, randomFileName

import statlib.stats, statlib.pstat
                           
CV_FOLDS = 5
SIGNIFICANCE_LEVEL = 0.08

def MeanError(errors):
    return 1.0 * sum(errors) / len(errors)
 
class MatrixNETFeatureEstimator:
    def __init__(self, path):
        self.path = path
        self.options = {'iterations':100}
        
    def LearnWithCrossValidation(self, train_filename):
        learn_errors = []
        test_errors = []
        
        for cv_index in range(CV_FOLDS):
            print "%d fold" % (cv_index + 1)
            
            cmdstring = self.path + '//matrixnet -F -n 3'
            cmdstring += " -f " + train_filename
                
            cmdstring += " -X %d/%d" % (cv_index + 1, CV_FOLDS)
            if self.options['iterations'] > 0:
                cmdstring += " -i " + str(self.options['iterations'])
            cmdstring += " > /dev/null"
        
            os.system(cmdstring)
            
            learn_error, test_error = self.GetError()
            learn_errors.append(learn_error)
            test_errors.append(test_error)
            
        return learn_errors, test_errors
        
    def GetError(self):
        with open("matrixnet.log") as log:
            for line in log:
                fields = line.split('\t')
                learn_error, test_error = 1 - lg(float(fields[0])), 1 - lg(float(fields[1]))
        return learn_error, test_error
 
def getOptions():
    usage = "usage: %prog config [options]"
    parser = OptionParser(usage)
    
    parser.add_option("-d", "--data", dest="datafile", action="store",
                      help="file with judgments. <electfile> in config", metavar="FILE")
    parser.add_option("--matrixnet", dest="matrixnet", action="store",
                      help="MatrixNET path in your system", metavar="PATH")
    parser.add_option("-i", "--iterations", action="store", type="int", default = -1, dest="iterations",
                      help="MatrixNET iterations count. While not set, uses MatrixNET default")
    parser.add_option("-o", "--output", dest="result_path", action="store", default = "feature_selection_out.txt", 
                      help="PATH", metavar="PATH")
    parser.add_option("-x", "--exclude", dest="exclude", action="append", default = [],
                      help="don't use FACTOR in feature selection", metavar="FACTOR")
                      
    (options, args) = parser.parse_args()
        
    return options, readConfig(args[0])
    
def GetResultString(is_significant, min_error_factor_name, cv_errors, confidence_probability):
        cv_error = MeanError(cv_errors)
        return  "------------------------------------------------------------------------------------------------------------\
               \nsignificance: %(0)s \t factor: %(1)s \t cv_error: %(2)f \t confidence probability: %(3)f \t %(4)s \t%(5)s \n\
------------------------------------------------------------------------------------------------------------\n\
               " % { "0": repr(is_significant), "1": min_error_factor_name, "2": cv_error, "3":100-confidence_probability, "4":repr(cv_errors), "5":statlib.stats.lvariation(cv_errors) }
        
def FeatureSelection(datafile, output_file_name, exclude_factors):
    output_file = open(output_file_name, "w")
    
    criteria = map(float, config['criteria'])
    coefs = {'query_info':criteria[0],'content':criteria[1], 'readability':criteria[2]}
    resultfile = randomFileName()
    
    getter = FactorsGetter(datafile)
    all_factors = getter.Get()
        
    for ex in exclude_factors:
        all_factors.remove(ex)
    
    estimator = MatrixNETFeatureEstimator(config['matrixnet']) if not options.matrixnet else MatrixNETFeatureEstimator(options.matrixnet)
        
    used_factors = []
    used_factors_confidence_probabilities = []
    used_factors_cv_errors = []
    
    learn_errors = [100 for i in range(CV_FOLDS)]
    test_errors = [100 for i in range(CV_FOLDS)]
    
    while len(used_factors) < len(all_factors):
        min_learn_errors = [100 for i in range(CV_FOLDS)]
        min_test_errors = [100 for i in range(CV_FOLDS)]
        min_error_factor_name = ""
        
        for factor_name in all_factors:
            if used_factors.count(factor_name) > 0:
                continue
                
            cur_factors = list(used_factors)
            cur_factors.append(factor_name)
                
            preparer = LearningDataFromXmlPreparer(datafile, coefs, lambda s,c: True, cur_factors, set([]))
            data = preparer.GetPreparedData()
            
            data.ToTreenet(resultfile)
            data.ToMatrixnetPairs(resultfile + '.pairs')
                
            cur_learn_errors, cur_test_errors = estimator.LearnWithCrossValidation(resultfile)
            if MeanError(cur_test_errors) < MeanError(min_test_errors):
                min_learn_errors = cur_learn_errors
                min_test_errors = cur_test_errors
                min_error_factor_name = factor_name
            print "\nfactor:%(0)s \t cv_error:%(1)f\n" % {"0": factor_name, "1": MeanError(cur_test_errors)}
            
        t_test_value, confidence_probability = statlib.stats.ttest_ind(test_errors, min_test_errors)
        is_significant = confidence_probability < SIGNIFICANCE_LEVEL
        
        test_errors = min_test_errors
        
        used_factors.append(min_error_factor_name)
        used_factors_cv_errors.append(MeanError(min_test_errors))
        used_factors_confidence_probabilities.append(100 - confidence_probability)
        
        result_str = GetResultString(is_significant, min_error_factor_name, min_test_errors, confidence_probability)
        print result_str
        print >> output_file, result_str
    
    print >> output_file, "############################\Overall results############################\n\
    Feature\tCV error\t Confidence probability"
    for index in range(len(used_factors)):
        print >> output_file, "%s\t%f\t%f\n" % (used_factors[index], used_factors_cv_errors[index], used_factors_confidence_probabilities[index])
    
    os.remove(resultfile)
    os.remove(resultfile + '.pairs')
    os.remove("matrixnet.log")
    
    output_file.close()
   
if __name__=="__main__":
    options, config = getOptions()
    
    FeatureSelection(options.datafile, options.result_path, options.exclude)
