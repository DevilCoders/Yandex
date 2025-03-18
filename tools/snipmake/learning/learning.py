#!/usr/bin/env python
# -*- coding: utf8 -*-

import learning_utils

from sys import argv, path
from learning_utils import LearningDataFromXmlPreparer, FactorsGetter, factorBoost, getLinearFormula, getFilter, saveTrainData

from learning_tools import LearningData, LearningObject, SVMLightLearner, LibSVMLearner, MatrixNETLearner, \
                           SVMModel, LibSVMModel
                           
from candidate_parser import SnippetsData
from candidate_tools import CandidateUpdaterByText, CandidateUpdaterByCoordinates
path.append("../snippet_xml_parser/python")

from classify import fillData, FirstGroupError, PairClassificationError, readIncFile
from common import *
from optparse import OptionParser
from os import remove
from copy import copy
                        
def prepareData(datafile, coefs, candidates_filter, factors, zeroFactors = set([]), separated_assessment = False):
    data = LearningData()
    data.ranking = True
    minids = {}
    minid = 0
    queryObjs = []
    with open(datafile) as datafile:
      for line in datafile:
          candidate = SnippetsData(line)
          if not candidates_filter(candidate):
              continue
          obj = LearningObject()
          try:
              obj.target = sum([candidate.marks[mark] * coefs[mark] for mark in coefs.iterkeys()]) / sum(coefs.values())
          except:
              continue
              
          obj.comment = ' '.join([candidate.id, candidate.url, candidate.query, candidate.scoords])
          if separated_assessment:
              keyid = (candidate.id, candidate.assessor)
          else:
              keyid = candidate.id
          if keyid not in minids:
              minids[keyid] = minid
              minid += 1
          obj.id = minid
          obj.weight = minid
          for k in range(len(factors)):
              try:
                obj.features[k + 1] = candidate.features[factors[k]] if factors[k] not in zeroFactors else 0.0
              except KeyError as e:
                  print e
                  print factors[k]
                  print candidate.features
          data.objects.append(obj)
    return data

    
def calculate(data, resultfile, learner, findc, cv_folds = 1, cv_times = 1, formula = 'class', pairwiseBlidnPath = "", iterations = -1, verbose = False):
    def learn(learn_type):
        learner.learn_type = learn_type
        saveTrainData(LEARNER, data, resultfile, pairwiseBlidnPath)
            
        print LEARNER, learn_type
        if cv_folds == 1 and (learn_type == 'optimization' or LEARNER == "MatrixNET"):
            print 'attention! leave-one-out doesn\'t work correctly with optimization and MatrixNET'
        if findc:
            c, error = learner.FindBestC(resultfile, cv_folds, cv_times, verbose)
        else:
            if LEARNER is "MatrixNET":
                learner.options['iterations'] = iterations
            else:
                learner.GetDefaultCParam(resultfile)
            if cv_times == 0:
                error = 0
            elif cv_folds == 1:
                error = learner.LeaveOneOut(resultfile, verbose)[1]
            else:
                errors = []
                for t in range(cv_times):
                    errors.extend(learner.CrossValidation(resultfile, cv_folds, verbose))
                error = sum(errors) / len(errors)
                
        if LEARNER is "MatrixNET":
            learner.Learn(resultfile, resultfile, None, log_filename = resultfile+'.log')
            learn_error = learner.GetError(learn = True)[0]
            remove('matrixnet.log')
        else:
            learner.Learn(resultfile, resultfile + '.model', log_filename = resultfile+'.log')
            learn_error = learner.Validation(resultfile, None, verbose)
            
        print '%.5f' % (error) + ' cv errors,', '%.5f' % (learn_error) + ' learn errors'
        return True if LEARNER is "MatrixNET" else resultfile + '.model'
        
    if verbose:
        print len(data.objects), 'objects in learning set'
    if len(data.objects) == 0:
        return False
    if formula == 'rank':
        return learn('ranking')
    if formula == 'pfound':
        return learn('pfound')
    data.ConvertRankingToPairClassification()
    if verbose:
        print len(data.objects), 'pairs in learning set'
    if len(data.objects) == 0:
        return False
    if formula == 'class':
        return learn('classification')
    if formula == 'pos':
        data.ConvertClassificationToOptimization(True)
        return learn('optimization')
   
def getOptions():
    usage = '''usage: %prog <config> [options]'''
    parser = OptionParser(usage)
    parser.add_option("--svmlight", dest="svmlight", action="store",
                      help="SVM-light path in your system", metavar="PATH")
    parser.add_option("--matrixnet", dest="matrixnet", action="store",
                      help="MatrixNET path in your system", metavar="PATH")
    parser.add_option("--liblinear", dest="liblinear", action="store",
                      help="LibLinear path in your system", metavar="PATH")
    parser.add_option("-u", "--update", dest="update", action="store",
                      help="update candidates' features with features from CANDFILE", metavar="CANDFILE")
    parser.add_option("--ut", dest="updateByText", action="store_true", default = False,
                      help="compare updated candidates by text")
    parser.add_option("-t", "--test", action="store_true",
                      help="test formula. With no model, use ranks as formula representation")
    parser.add_option("-m", "--model", action="store", metavar="INCFILE",
                      help="test linear formula, described in INCFILE")
    parser.add_option("--separated", action="store_true",
                      help="learn separately on each criteria")
    parser.add_option("--cv", action="store", type="int", nargs=2,
                      help="N-times K-folds cross-validation. (1 1): leave-one-out, (0 0): no cv", metavar="N K")
    parser.add_option("-C", "--findc", action="store_true",
                      help="find best C parameter for svm classifier")
    parser.add_option("-F", "--fast", action="store_true",
                      help="fast learning: no cross_validation, same to [--cv 0 0]")
    parser.add_option("-B", "--factor_boost", action="store_true",
                      help="factor boost")
    parser.add_option("-M", "--MN", action="store_true",
                      help="use MatrixNET")
    parser.add_option("-L", "--LL", action="store_true",
                      help="use LibLinear")
    parser.add_option("-i", "--iterations", action="store", type="int", default = -1,
                      help="MatrixNET iterations count. While not set, uses MatrixNET default")
    parser.add_option("-n", "--normalize", action = "store_true",
                      help="normalize features")
    parser.add_option("-s", "--scale", action = "store_true",
                      help="scale features. [0..1] - limits, use --scaleopts to change")
    parser.add_option("--scaleopts", action = "store", type="float", nargs=2, default = (0,1),
                      help="lower and upper limits for scaling", metavar="LOWER UPPER")
    parser.add_option("-d", "--data", dest="datafile", action="store",
                      help="file with judgments. <electfile> in config", metavar="FILE")
    parser.add_option("--pool", action = "store_true",
                      help="filter results by <pool> in config")
    parser.add_option("-G", action = "store", dest="generate_path", help="generate only training data files", metavar="PATH")
    parser.add_option("--pairwise", action = "callback", dest="pairwise", default = "", type="str",
                      help="Blind file PATH for matrixnet pairwise classifiaction",  metavar="PATH", callback=learning_utils.pairwise_option_callback)
    parser.add_option("-f", "--factors", dest="factors", action="append",
                      help="use FACTOR in learning. Without this option learn on all factors", metavar="FACTOR")
    parser.add_option("-x", "--exclude", dest="exclude", action="append", default = [],
                      help="don't use FACTOR in learning", metavar="FACTOR")
    parser.add_option("-z", "--zerofactor", dest="zerofactor", action="append", default = [],
                      help="set feature value to zero to all objects", metavar="ZEROFACTOR")
    parser.add_option("-o", "--output", dest="result_path", action="store",
                      help="Data, models, formulae will be stored in PATH", metavar="PATH")
    parser.add_option("-l", "--learning", dest="formula", action="store",
                      type="choice", choices=["class","rank","pos","pfound"], default="class",
                      help="Learning type: class (default), pos, rank")
    parser.add_option("-v", "--verbose", action = "store_true",
                      help="print additional information")
                      
    (options, args) = parser.parse_args()
    if len(args) > 1:
        parser.error("incorrect number of arguments")
    if options.normalize and options.scale:
        parser.error("normalization (-n) and scaling (-s) are mutually exclusive")
    if options.MN and options.LL:
        parser.error("use SVMLight(default), MatrixNET (-M) or LibLinear (-L)")
    if options.fast and options.cv:
        parser.error("fast (-F) learning blocks cross-validation (--cv)")
    if options.findc and options.MN:
        parser.error("no find best C parameter (-C) option for MatrixNET")
    return options, args
        
if __name__=="__main__":
    options, args = getOptions()
    config = readConfig(args[0] if len(args) else path[0] + '/learning.cfg')
    MODE = 'learning'
    if options.test:
        MODE = 'test'
    elif options.update:
        MODE = 'update'
    elif options.factor_boost:
        MODE = 'factor_boost'
    
    resultpath = config['resultpath'] if not options.result_path else options.result_path
    datafile = config['electfile'] if not options.datafile else options.datafile
    criteria = map(float, config['criteria'])
    coefs = {'query_info':criteria[0],'content':criteria[1], 'readability':criteria[2]}
    
    if options.pool or config['pool']:
        pools = options.pool if options.pool else config['pool']
        if isinstance(pools, str):
            if pools.find("|") != -1:
                pools = map(strip, pools.split("|"))
            else:
                pools = [pools,]
    else:
        pools = []
        
    filter_function = getFilter(pools)
    
    LEARNER = "SVMLight"
    if options.MN:
        LEARNER = "MatrixNET"
    if options.LL:
        LEARNER = "LibLinear"
    if options.fast:
        cv_folds = 0
        cv_times = 0
    else:
        cv_folds = int(config['cv_folds']) if not options.cv else options.cv[1]
        cv_times = int(config['cv_times']) if not options.cv else options.cv[0]
    formula = config['learning'] if not options.formula else options.formula
    
    getter = FactorsGetter(datafile)
    allfactors = getter.Get()
    
    originalfactors = copy(allfactors)
    zeroFactors = set([])
    for ex in options.exclude:
        allfactors.remove(ex)
    for ex in options.zerofactor:
        zeroFactors.add(ex)
    factors = options.factors if options.factors else allfactors

    if LEARNER is "MatrixNET":
        learner = MatrixNETLearner(config['matrixnet']) if not options.matrixnet else MatrixNETLearner(options.matrixnet)
    elif LEARNER is "LibLinear":
        learner = LibSVMLearner(config['liblinear']) if not options.liblinear else LibSVMLearner(options.liblinear)
    else:
        learner = SVMLightLearner(config['svmlight'] if 'svmlight' in config else config['svmpath']) if not options.svmlight else SVMLightLearner(options.svmlight)    
    
    def simple_learning():
        if options.normalize:
            data.Normalize()
        elif options.scale:
            data.ScaleFeatures(options.scaleopts[0], options.scaleopts[1])
        model_file_name = calculate(data, resultfile, learner, options.findc, cv_folds, cv_times, formula, options.pairwise, options.iterations, options.verbose)
        if not model_file_name:
            return dict([[feature, 0] for feature in factors])
        if LEARNER is "MatrixNET":
            exit()
        elif LEARNER is "LibLinear":
            model = LibSVMModel(model_file_name)
        else:
            model = SVMModel(model_file_name)
        
        return getLinearFormula(model, data, factors, options.normalize, options.scale, options.verbose)

    if MODE is 'update':
        resultfile = datafile + '.updated_' + options.update[max(options.update.rfind('/'), options.update.rfind('\\'))+1:]
        print 'Update', datafile, 'with factors from', options.update
        print 'Result to', resultfile
        
        updater = CandidateUpdaterByCoordinates(datafile, options.update, resultfile)
        if options.updateByText: 
            updater = CandidateUpdaterByText(datafile, options.update, resultfile)
        updater.Update()
        
    elif MODE is 'factor_boost':
        factorBoost(datafile, coefs, learner, filter_function, allfactors, options.factors, cv_folds, cv_times, formula, options.normalize, options.verbose)
    elif MODE is 'test':
        if not options.model:
            raise Exception("Model file is not specified.")
            
        preparer = LearningDataFromXmlPreparer(datafile, coefs, filter_function, factors, zeroFactors)
        data = preparer.GetPreparedData()
        collection = fillData(data, options.model, LEARNER, learner)
        tasks_bad, tasks_all = FirstGroupError(collection, "predicted" if not options.model else None)
        pairs_bad, pairs_all = PairClassificationError(collection, "predicted" if not options.model else None)
        print "# Pair classification error = {0}% ({1}/{2})".format(100*float(pairs_bad) / float(pairs_all), pairs_bad, pairs_all)
        print "# First best sample error   = {0}% ({1}/{2})".format(100*float(tasks_bad) / float(tasks_all), tasks_bad, tasks_all)
    else:
        resultfile = '_'.join([resultpath, LEARNER, formula])
        
        # !!!! Don't support Separated mode
        if options.separated:
            features = dict([[factor,0] for factor in originalfactors])
            active_features = dict([[factor, None] for factor in allfactors])
            
            for criteria_name, criteria_mult in coefs.iteritems():
                if criteria_mult == 0:
                    continue
                print criteria_name
                
                data = prepareData(datafile, dict([[criteria_name, 1.0]]), filter_function, factors, zeroFactors, True)                
                                
                criteria_features = simple_learning()
                for name in features:
                    if name in active_features:
                        features[name] += criteria_features[name] * criteria_mult / sum(coefs.values())
                        
        else:
            preparer = LearningDataFromXmlPreparer(datafile, coefs, filter_function, factors, zeroFactors)
            data = preparer.GetPreparedData()
            
            if len(options.generate_path) > 0:
                saveTrainData(LEARNER, data, options.generate_path, options.pairwise)
                print "Train data has been saved to " + options.generate_path
                exit()
            else:
                features = simple_learning()
            
        print 'Formula: ', resultfile + '.inc'
        makeHeader(features, originalfactors, resultfile + '.inc')
        