#!/usr/bin/env python
# -*- coding: utf8 -*-

from sys import argv, path
from learning_tools import LearningData, LearningObject, CalculateQDPairHash
                           
from candidate_parser import SnippetsData
from candidate_tools import CandidateUpdaterByText, CandidateUpdaterByCoordinates
from candidate_tools import CandidateUpdaterBase
path.append("../snippet_xml_parser/python")
from snippet_dump_xml import SnippetDumpXmlReader
from snippet_dump_xml import SnippetDumpXmlReaderStopParseException

from classify import fillData, FirstGroupError, PairClassificationError, readIncFile
from common import *
from optparse import OptionParser
from os import remove
from copy import copy

#### ------------ Options utils ------------ ####

def pairwise_option_callback(option, opt_str, value, parser):
    parser.values.pairwise = value
    parser.values.formula = "rank"

#### ------------ Data preparing utils ------------ ####

class LearningDataFromXmlPreparer:
    def __init__(self, datafile, coefs, candidates_filter, factors, zeroFactors = set([]), separated_assessment = False):
        self.datafile = datafile
        self.coefs = coefs
        self.candidates_filter = candidates_filter
        self.factors = factors
        self.zeroFactors = zeroFactors
        self.separated_assessment = separated_assessment
        
        self.learningData = None
        self.minids = {}
        self.minid = 0
    
    def GetPreparedData(self):
        self.snippetCount = 0
        self.learningData = LearningData()
        self.learningData.ranking = True

        dumpXmlParser = SnippetDumpXmlReader(self.datafile, self.ProcessCandidateCallback)
        dumpXmlParser.StartParse()
        
        return self.learningData
        
    def ProcessCandidateCallback(self, poolID, qdPairData, snippetData):
        candidate = self.CreateSnippetsData(qdPairData, snippetData)
        if not self.candidates_filter(candidate, poolID):
            return
            
        obj = LearningObject()
        
        try:
            obj.target = sum([candidate.marks[mark] * self.coefs[mark] for mark in self.coefs.iterkeys()]) / float(sum(self.coefs.values()))
        except:
            return
            
        obj.comment = ' '.join([candidate.id, candidate.url.encode("utf8"), candidate.query.encode("utf8"), candidate.scoords.encode("utf8")])
        obj.qdpair_hash = CalculateQDPairHash(candidate.query.encode("utf8"), candidate.url.encode("utf8"))
        obj.scoords = candidate.scoords

        if self.separated_assessment:
            keyid = (candidate.id, candidate.assessor)
        else:
            keyid = candidate.id
        if keyid not in self.minids:
            self.minids[keyid] = self.minid
            self.minid += 1
        obj.id = self.minid
        obj.weight = self.minid

        for k in range(len(self.factors)):
            obj.features[k + 1] = candidate.features[self.factors[k]] if self.factors[k] not in self.zeroFactors else 0.0
        self.learningData.objects.append(obj)        
        
    def CreateSnippetsData(self, qdPairData, xmlSnippetData):
        snippetData = SnippetsData()
        snippetData.id = CandidateUpdaterBase.GetQDPairHash(qdPairData)
        snippetData.query = qdPairData.query
        snippetData.region = qdPairData.region
        snippetData.url = qdPairData.url
        snippetData.relevance = qdPairData.relevance
        snippetData.title = xmlSnippetData.title
        snippetData.snippet = CandidateUpdaterByText.GetSnippetText(xmlSnippetData)
        snippetData.algorithm = xmlSnippetData.algorithm
        
        # TODO support separate mode
        for fragment in xmlSnippetData.fragments:
            snippetData.coords.append(fragment.coords)
            snippetData.scoords += fragment.coords + " "
            
        snippetData.scoords = snippetData.scoords.strip()
        snippetData.rank = xmlSnippetData.rank
        snippetData.sfeatures = xmlSnippetData.featuresString
        snippetData.ParseFeaturesString()
       
        for mark in xmlSnippetData.marks:
            snippetData.marks[mark.criteria] = float(mark.value)
            
        return snippetData

class FactorsGetter:
    def __init__(self, datafile):
        self.dumpXmlParser = SnippetDumpXmlReader(datafile, self.AlgoCallback)
        self.featureString = ""
        
    def Get(self):
        try:
            self.dumpXmlParser.StartParse()
        except SnippetDumpXmlReaderStopParseException:
            features, featnames = SnippetsData.ParseFeaturesStringInternal(self.featureString)
            return featnames
        return []
        
    def AlgoCallback(self, poolID, qdPairData, xmlSnippetData):
        self.featureString = xmlSnippetData.featuresString
        self.dumpXmlParser.StopParseByRaiseException()

def getFilter(pools):
    poolsHash = dict([(pool, None) for pool in pools])
    def filter_function(candidate, poolID = ""):
        retValue = False
        if len(poolsHash) > 0:
            retValue = poolID in poolsHash
        return retValue
        
    return filter_function

def saveTrainData(learner_type, data, resultfile, pairwiseBlidnPath):
    if learner_type is "MatrixNET":
        data.ToTreenet(resultfile)
        data.ToMatrixnetPairs(resultfile + '.pairs', pairwiseBlidnPath)
    elif learner_type is "LibLinear":
        data.ToInverseSVM(resultfile, skipComment = True)
    else:
        data.ToSVM(resultfile)

#### ------------ Learning utils ------------ ####

def factorBoost(datafile, coefs, svm_learner, candidates_filter, allfactors, factors, cv_folds = 5, cv_times = 1, formula = 'class', norm = False, verbose = False):
    tempfile = 'temp_sdfbsdfhsd'
    def getError():
        if norm:
            data.Normalize()
        if formula == 'rank':
            learn_type = 'ranking'
        elif formula == 'class':
            data.ConvertRankingToPairClassification()
            learn_type = 'classification'
        elif formula == 'pos':
            data.ConvertRankingToPairClassification()
            data.ConvertClassificationToOptimization(True)
            learn_type = 'optimization'
        data.ToSVM(tempfile)
        svm_learner.learn_type = learn_type
        c = svm_learner.GetDefaultCParam(tempfile)
        test_errors = []
        for t in range(cv_times):
            test_errors.extend(svm_learner.CrossValidation(tempfile, cv_folds, c, verbose)[1])
        return sum(test_errors) / len(test_errors)
    
    preparer = LearningDataFromXmlPreparer(datafile, coefs, candidates_filter, allfactors)
    data = preparer.GetPreparedData()    
    
    all_err = getError()
    if factors:
        prefactors = list(set(allfactors) - set(factors))
        
        preparer = LearningDataFromXmlPreparer(datafile, coefs, candidates_filter, prefactors)
        data = preparer.GetPreparedData()
        
        pre_err = getError()
        print 'Error with factors "' + '", "'.join(prefactors) + '": %.4f' % pre_err
    else:
        factors = allfactors
        pre_err = 1.0
        prefactors = []
    if len(factors) > 1:
        for factor in factors:
            preparer = LearningDataFromXmlPreparer(datafile, coefs, candidates_filter, prefactors + [factor])
            data = preparer.GetPreparedData()
                        
            err = getError()
            print 'Error with factor "' + factor + '": %.4f' % err
            print 'BOOST: %.4f' % (pre_err - err)
    print 'Error with factors "' + '". "'.join(allfactors) + '": %.4f' % all_err
    print 'BOOST: %.4f' % (pre_err - all_err)
    remove(tempfile)

def getLinearFormula(model, learning_data, featnames, norm = False, scale = False, verbose = False):
    from math import sqrt
    linear_kernel_coefs = model.GetLinearKernelCoefs()
    
    features = dict([[featnames[num], linear_kernel_coefs[num+1]] for num in range(len(featnames))])
    if verbose:
        print '\t'.join([name+':%.3f' % value for name, value in sorted(features.iteritems(), key=lambda x:-abs(x[1]))])
    if norm:
        sigma = dict(map( lambda x:[x[0], sqrt(x[1]) if x[1] else 1.0], learning_data.disps.iteritems() ))
        features = dict([[featnames[num], linear_kernel_coefs[num+1] / sigma[num+1]] for num in range(len(featnames))])
    elif scale:
        for num in range(len(featnames)):
            featureName = featnames[num]
            if learning_data.featureMin[num+1] == learning_data.featureMax[num+1]:
                continue
            features[featureName] = features[featureName]*(learning_data.upperFeatureBoundary - learning_data.lowerFeatureBoundary) / (learning_data.featureMax[num+1] - learning_data.featureMin[num+1])
    return features
    