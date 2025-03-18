#!/usr/bin/env python
# -*- coding: utf8 -*-

from math import sqrt, ceil, exp, log as lg
from math import sqrt, ceil, exp
from random import shuffle, choice
from shutil import copyfile
from sys import maxint

import hashlib
import os
import random
import re
import uuid

from candidate_tools import CandidateUpdaterUtils

#### ------------ Text utils ------------ ####

class TextUtils:
    @staticmethod
    def ClearHtml(text):
        cleared = re.sub(u"(<.*?>)|(\&quot;)|(\&nbsp;)", u"", text)
        return cleared

    @staticmethod
    def ClearBadSymbols(text):
        cleared = re.sub(u"\n", u"", text)
        return cleared

    @staticmethod
    def ToUnicode(text):
        if not isinstance(text, unicode):
            return text.decode("utf-8")
        return text

    @staticmethod
    def ClearText(text):
        cleared = TextUtils.ToUnicode(text)
        cleared = TextUtils.ClearHtml(cleared)
        cleared = TextUtils.ClearBadSymbols(cleared)
        return cleared

#### ------------ Math utils ------------ ####

def mean(vector):
    return sum(vector) / float(len(vector))

def disp(vector):
    if len(vector) == 1:
       return 0
    m = mean(vector)
    res = 0
    for item in vector: 
       res += (item - m)**2
    return res / float(len(vector) - 1)

def randomFileName():
    return uuid.uuid1().hex

def CalculateQDPairHash(query, url):
    url = TextUtils.ToUnicode(url).lower().encode("utf-8")
    query = TextUtils.ToUnicode(query).lower().encode("utf-8")
    
    return hashlib.sha224("_".join( [CandidateUpdaterUtils.HtmlEntityDecodeFull(query), CandidateUpdaterUtils.UrlNormalize(url)])).hexdigest()

#### ------------ Learners ------------ ####

class MatrixNETLearner:
    def __init__(self, path):
        self.path = path
        self.learn_type = ''
        self.options = {'iterations':-1}
    
    def Learn(self, train_filename, model = None, test_filename = None, log_filename = None):
        '''with no <log_filename> uses standart output'''
        cmdstring = self.path + '//matrixnet'
        if self.learn_type == 'classification':
            cmdstring += " -c"
        elif self.learn_type == 'ranking':
            cmdstring += " -P"
        elif self.learn_type == 'pfound':
            cmdstring += " -F -n 3"
        else:
            raise TypeError, "Unknown learning type"
        cmdstring += " -f " + train_filename
        if test_filename:
            cmdstring += " -t " + test_filename
        if model:
            cmdstring += " -o " + model
        if self.options['iterations'] > 0:
            cmdstring += " -i " + str(self.options['iterations'])
        if log_filename:
            cmdstring += " > " + log_filename
        os.system(cmdstring)
    
    def GetError(self, learn = False):
        defname = "matrixnet"
        with open(defname + '.log') as log:
            resline = log.readlines()[-1].split('\t')
            if self.learn_type == 'pfound':
                lerror, terror = 1 - lg(float(resline[0])), 1 - lg(float(resline[1])) if not learn else None
            else:
                lerror, terror = 1 - float(resline[2]), 1 - float(resline[3]) if not learn else None
        return lerror, terror

    def Classify(self, test_filename, model_filename, res_filename = None, log_filename = None):
        copyfile(model_filename, "matrixnet.bin")
        cmdstring = self.path + "//matrixnet -A -f " + test_filename
        os.system(cmdstring)
        os.remove("matrixnet.bin")
        os.rename(test_filename + ".matrixnet", res_filename)        
       
    def Clear(self, results = None):
        defname = "matrixnet"
        if not results:
            results = defname
        os.remove(results + '.bin')
        os.remove(results + '.inc')
        os.remove(results + '.fpair')
        os.remove(results + '.fstr')
        os.remove(defname + '.log')
        
    def Validation(self, train_file_name, test_file_name = None, verbose = False):
        '''Learn on train, validate on test'''
        logfile = randomFileName()
        if verbose:
            print 'Learn on', train_file_name
            if test_file_name:
                print 'Test on', test_file_name 
        self.Learn(train_file_name, None, test_file_name, logfile)
        lerror, terror = self.GetError()
        self.Clear()
        os.remove(logfile)
        return terror if test_file_name else lerror
    
    def CrossValidation(self, source_file_name, folds = 2, verbose = False):
        '''Run n-folds cross-validation'''
        errors = []
        if verbose:
            print ' ' + str(folds) + '-folds cross-validation check...'
        data = LearningData()
        data.LoadTreenet(source_file_name)
        data_parts = [LearningData() for i in range(folds)]
        tfname = randomFileName()
        lfname = randomFileName()
        
        if self.learn_type == "ranking" or self.learn_type == "pfound":
            objs = {}
            for obj in data.objects:
                if obj.id not in objs:
                    objs[obj.id] = []
                objs[obj.id].append(obj)
            for id, objects in objs.iteritems():
                choice(data_parts).objects += objects
        elif self.learn_type == "classification":
            for obj in data.objects:
                choice(data_parts).objects.append(obj)
        
        for i in range(len(data_parts)):
            if verbose:
                print ' ' + str(i+1),
            learn_data = LearningData()
            for j in range(len(data_parts)):
                if i != j:
                    learn_data.objects.extend(data_parts[j].objects)
            data_parts[i].ToTreenet(tfname)
            learn_data.ToTreenet(lfname)
            if self.learn_type == "ranking":
                data_parts[i].ToMatrixnetPairs(tfname + '.pairs')
                learn_data.ToMatrixnetPairs(lfname + '.pairs')
            errors.append(self.Validation(lfname, tfname))
            os.remove(tfname)
            os.remove(tfname + '.matrixnet')
            os.remove(lfname)
            os.remove(lfname + '.matrixnet')
            if self.learn_type == "ranking":
                os.remove(tfname + '.pairs')
                os.remove(lfname + '.pairs')
        if verbose:
            print
        return errors
        
class SVMLightLearner:
    def __init__(self, path):
        self.path = path
        self.learn_type = ''
        self.options = {'c':-1, 'leave_one_out':False}
        '''<c> - parameter of slack variables. Can be estimated by FindBestC
        set <leave_one_out> to True to use builtin leave-one-out quality check'''
        
    def Learn(self, train_filename, model_filename = None, log_filename = None):
        '''with no <log_filename> uses standart output'''
        if not model_filename:
            model_filename = train_filename + '.model'
        cmdstring = self.path + "//svm_learn "
        if self.learn_type == "ranking":
            cmdstring += "-z p "
        elif self.learn_type == "classification":
            cmdstring += "-z c -b 0 "
        elif self.learn_type == "optimization":
            cmdstring += "-z o "
        else:
            raise TypeError, "Unknown learning type"
        if self.options['c'] >= 0:
            cmdstring += "-c " + str(self.options['c']) + " "
        if self.options['leave_one_out']:
            cmdstring += "-x 1 "
        cmdstring += train_filename + " " + model_filename
        if log_filename:
            cmdstring += " > " + log_filename
        os.system(cmdstring)

    def Classify(self, test_filename, model_filename, res_filename = None, log_filename = None):
        if not res_filename:
            res_filename = test_filename + '.svmlight'
        cmdstring = self.path + "//svm_classify " + test_filename + " " + model_filename + " " + res_filename
        if log_filename:
            cmdstring += " > " + log_filename
        os.system(cmdstring)
        
    def GetError(self, test_filename, res_filename, log_filename):
        if self.learn_type == "ranking":
            return self.GetRankingError(test_filename, res_filename)
        elif self.learn_type == "classification":
            return self.GetClassificationError(log_filename)
        elif self.learn_type == "optimization":
            return self.GetOptimizationError(res_filename)
        else:
            raise TypeError, "Unknown learning type"
        
    def GetRankingError(self, test_file, results_file):
        with open(test_file) as correct_file:
            correct = correct_file.readlines()
        with open(results_file) as results_file:
            results = results_file.readlines()
        
        curid = -1
        error = 0
        all = 0
        objs_per_id = []
        rank_per_id = []
        for index in range(len(correct)) + [0]:
            obj = LearningObject()
            obj.LoadFromSVMFormat(correct[index])
            if curid != obj.id:
                curid = obj.id
                for i in range(0, len(objs_per_id)):
                    for j in range(i+1, len(objs_per_id)):
                        if (i == j) or (objs_per_id[i].target == objs_per_id[j].target):
                            continue
                        if cmp(rank_per_id[i], rank_per_id[j]) != cmp(objs_per_id[i].target, objs_per_id[j].target):
                            error += 1
                        all += 1
                objs_per_id = []
                rank_per_id = []
            objs_per_id.append(obj)
            rank_per_id.append(float(results[index]))
        return float(error)/all
    
    def GetOptimizationError(self, results_file):
        all = 0
        error = 0
        with open(results_file) as results:
            for line in results.readlines():
                all += 1
                if float(line.strip()) <= 0:  #working correctly till all examples have positive weights
                    error += 1
        return float(error)/all
    
    def GetClassificationError(self, logfile):
        with open(logfile) as log:
            info = log.readlines()[3].split()
        return float(info[-4]) / float(info[-2])
    
    def GetDefaultCParam(self, source_file_name):
        out_file = randomFileName()
        model_file = randomFileName()
        self.Learn(source_file_name, model_file, log_filename = out_file)
        with open(out_file, "r") as out:
            lines = out.readlines()
        os.remove(out_file)
        os.remove(model_file)
        if self.learn_type == "ranking":
            self.options['c'] = float(lines[3].strip().split("C=")[1])
        elif self.learn_type == "classification":
            self.options['c'] = float(lines[2].strip().split("C=")[1])
        elif self.learn_type == "optimization":
            self.options['c'] = float(lines[2].strip().split("C=")[1])
        else:
            raise TypeError, "Unknown learning type"
        return self.options['c']

    def FindBestC(self, source_file_name, folds = 5, times = 1, verbose = False):
        '''Finds best value for C parameter based on t-times n_fold cross validation or leave-one-out'''
        default_c_list = [2**-5, 2**-3, 2**-1, 2, 2**3, 2**5, 2**7]
        
        min_error = 100
        min_c = default_c_list[0]
    
        for c in default_c_list:
            terror = []
            self.options['c'] = c
            if verbose:
                print "C =", str(c),
                if times > 1:
                    print " " + str(times) + "-times cross-validation:" 
            if folds > 1:
                for t in range(times):
                    random.seed(20) # This hack prevents random splitting learning data
                    terror.extend(self.CrossValidation(source_file_name, folds, verbose))
                if verbose:
                    print "Test errors:", terror
                    print "Mean error:", mean(terror), "; st.deviation:", sqrt(disp(terror)) ,", min:", min(terror), ", max:", max(terror)
            else:
                terror = [self.LeaveOneOut(source_file_name, verbose)]
                if verbose:
                    print "Test error:", terror[0]
            if mean(terror) < min_error:
                min_error = mean(terror)
                min_c = c
        
        self.options['c'] = min_c
        return min_c, min_error
        
    def Validation(self, train_file_name, test_file_name = None, verbose = False):
        '''Learn on train, validate on test'''
        if not test_file_name:
            test_file_name = train_file_name
        model = randomFileName()
        resfile = randomFileName()
        logfile = randomFileName()
        if verbose:
            print 'Learn on', train_file_name
        self.Learn(train_file_name, model, logfile)
        if verbose:
            print 'Test on', test_file_name 
        self.Classify(test_file_name, model, resfile, logfile)
        error = self.GetError(test_file_name, resfile, logfile)
        os.remove(model)
        os.remove(resfile)
        os.remove(logfile)
        return error
    
    def LeaveOneOut(self, source_file_name, verbose = False):
        '''Run leave-one-out
        do not work with optimization!'''
        if verbose:
            print ' leave-one-out check...'
        log_file = randomFileName()
        model = randomFileName() 
        
        self.options['leave_one_out'] = True
        self.Learn(source_file_name, model, log_file)
        self.options['leave_one_out'] = False
        if self.learn_type == "optimization":
            error = 0
        else:
            with open(log_file, "r") as out:
                error = float(out.readlines()[-6].strip().split("=")[1].strip("%")) / 100
        os.remove(log_file)
        os.remove(model)
        return error
    
    def CrossValidation(self, source_file_name, folds = 2, verbose = False):
        '''Run n-folds cross-validation'''
        errors = []
        if verbose:
            print ' ' + str(folds) + '-folds cross-validation check...'
        data = LearningData()
        data.LoadSVM(source_file_name)
        data_parts = [LearningData() for i in range(folds)]
        tfname = randomFileName()
        lfname = randomFileName()
        
        if self.learn_type == "ranking":
            objs = {}
            for obj in data.objects:
                if obj.id not in objs:
                    objs[obj.id] = []
                objs[obj.id].append(obj)
            for id, objects in objs.iteritems():
                choice(data_parts).objects += objects
        elif self.learn_type == "classification":
            for obj in data.objects:
                choice(data_parts).objects.append(obj)
        elif self.learn_type == "optimization":
            data_part_learn = []
            for obj in data.objects:
                if obj.target == 0:
                    data_part_learn.append(obj)
                else:
                    choice(data_parts).objects.append(obj)
                      
        for i in range(len(data_parts)):
            if verbose:
                print ' ' + str(i+1),
            learn_data = LearningData()
            for j in range(len(data_parts)):
                if i != j:
                    learn_data.objects.extend(data_parts[j].objects)
            if self.learn_type == "optimization":
                learn_data.objects.extend(data_part_learn)
                
            if self.learn_type == "ranking":
                data_parts[i].ranking = True
                learn_data.ranking = True
            data_parts[i].ToSVM(tfname)
            learn_data.ToSVM(lfname)
            errors.append(self.Validation(lfname, tfname))
            os.remove(tfname)
            os.remove(lfname)
        if verbose:
            print
        
        return errors

class LibSVMLearner(SVMLightLearner):
    def __init__(self, path):
        self.path = path
        self.learn_type = ''
        self.options = {'c':-1, 'leave_one_out':False}
        
    def Learn(self, train_filename, model_filename = None, log_filename = None):
        if self.learn_type != "classification":
            raise TypeError, "LibSVMLearner supports classification task only"
        
        cmdstring = self.path + "//train -s 5 " # TODO add option s -6
        if not model_filename:
            model_filename = train_filename + '.model'
            
        if self.options['c'] >= 0:
            cmdstring += "-c " + str(self.options['c']) + " "
            
        cmdstring += train_filename + " " + model_filename
        
        if log_filename:
            cmdstring += " > " + log_filename
        
        os.system(cmdstring)

    def Classify(self, test_filename, model_filename, res_filename = None, log_filename = None):
        if not res_filename:
            res_filename = test_filename + '.libsvm'
        cmdstring = self.path + "//predict " + test_filename + " " + model_filename + " " + res_filename
        if log_filename:
            cmdstring += " > " + log_filename
        os.system(cmdstring)
        
    def GetError(self, test_filename, res_filename, log_filename):
        if self.learn_type == "classification":
            return self.GetClassificationError(log_filename)
        else:
            raise TypeError, "Unknown learning type"
            
    def GetClassificationError(self, logfile):
        with open(logfile) as log:
            info = log.readlines()[0].split()
        return 1 - float(info[2].replace("%", "")) / 100
            
    def GetDefaultCParam(self, source_file_name):
        pass

class LearningObject:
    def __init__(self, id = -1, target = 0, features = {}, weight = 1, comment = "", qdpair_hash = "", scoords = ""):
        self.id = id
        self.target = target
        self.features = {}
        for key in features.keys():
            self.features[key] = features[key]
        self.weight = weight        
        self.comment = comment
        
        # For matrixnet pairwise classsification
        self.qdpair_hash = qdpair_hash
        self.scoords = scoords
            
    def LoadFromSVMFormat(self, str):
        str,sep,self.comment = str.strip('\n').partition("#")
        self.target = float(str[:str.index(" ")]) #attention! may not work correctly on integers
        features = [f.split(":") for f in str.strip().split(" ")[1:]]
        if not all(len(f) == 2 for f in features):
            raise Exception, "Incorrect SVM format string"
        else:
            features = dict(features)
            if "qid" in features:
                self.id = int(features.pop("qid"))
            if "cost" in features:
                self.weight = float(features.pop("cost"))
            self.features.update((int(f[0]), float(f[1])) for f in features.iteritems())                          
            
    def LoadFromTreenetFormat(self,str):
        strList = str.strip('\n').split("\t")
        self.id = int(strList[0])
        self.target = float(strList[1]) #attention! may not work correctly on integers
        self.comment = strList[2]
        self.weight = float(strList[3])
        
        for index in range(4,len(strList)):
            self.features[index - 3] = float(strList[index])
    
    def ToSVMformat(self, ranking = False, weight = False, skipTarget = False, skipComment = False):
        res = ""
        if not skipTarget:
           res = str(self.target) + " "
        if ranking:
            res += "qid:"+ ("%(feat)d" % {"feat" : self.id})+" "
        if weight:
            if self.weight != 1:
                res += "cost:"+ ("%(feat)d" % {"feat" : self.weight})+" "        
        
        for key in sorted(self.features.keys()):
            res += str(key)+":"+ ("%(feat).7f" % {"feat" : self.features[key]})+" "
            
        if not skipComment:
           res += "# " + str(self.comment)
        return res
    
    def ToInverseSVMformat(self, ranking = False, weight = False, skipTarget = False, skipComment = False):
        res = ""
        if not skipTarget:
            target = -int(self.target)
            res = str(target) + " "
               
        if ranking:
            res += "qid:"+ ("%(feat)d" % {"feat" : self.id})+" "
        if weight:
            if self.weight != 1:
                res += "cost:"+ ("%(feat)d" % {"feat" : self.weight})+" "        
        
        for key in sorted(self.features.keys()):
            value = -float(self.features[key])
            res += str(key)+":"+ ("%(feat).7f" % {"feat" : value})+" "
        if not skipComment:
            res += "# " + str(self.comment)
        return res
        
    def ToLibSVMformat(self, ranking = False, weight = False, skipTarget = False):
        res = ""
        if not skipTarget:
           res = str(self.target) + " "
        if ranking:
            res += "qid:"+ ("%(feat)d" % {"feat" : self.id})+" "
        if weight:
            if self.weight != 1:
                res += "cost:"+ ("%(feat)d" % {"feat" : self.weight})+" "        
        
        for key in sorted(self.features.keys()):
            res += str(key)+":"+ ("%(feat).7f" % {"feat" : self.features[key]})+" "
        return res
    
    def ToTreenetFormat(self):
        res = str(self.id) + "\t" + str(self.target) + "\t"
        res += str(self.comment) + "\t" + str(self.weight)
        
        for feature in sorted(self.features.keys()):
            res += "\t" + ("%(feat).7f" % {"feat" : self.features[feature]})
            
        return res
    
    def ToBBRFormat(self, threshold):
        if self.target < threshold:
            res = "-1 "
        else:
            res = "1 "
        for key in sorted(self.features.keys()):
            res += str(key)+":"+ ("%(feat).7f" % {"feat" : self.features[key]})+" "
        res += "# " + str(self.comment)
        return res
        
class LearningData:
    def __init__(self):
        self.objects = []
        self.ranking = False
        self.weight = False
        
    def LoadSVM(self, fileName):
        with open(fileName, "r") as input:
            for obj in input:
                o = LearningObject()
                o.LoadFromSVMFormat(obj)
                self.objects.append(o)
                
    def LoadTreenet(self, fileName):
        with open(fileName, "r") as input:
            for obj in input:
                o = LearningObject()
                o.LoadFromTreenetFormat(obj)
                self.objects.append(o)

    def ToSVM(self, fileName, skipComment = False):
        with open(fileName, "w") as output:
            for obj in self.objects:
                output.write(obj.ToSVMformat(self.ranking, self.weight, skipComment = skipComment) + "\n")
                
    def ToInverseSVM(self, fileName, skipComment = False):
        with open(fileName, "w") as output:
            for obj in self.objects:
                output.write(obj.ToSVMformat(self.ranking, self.weight, skipComment = skipComment) + "\n")
                output.write(obj.ToInverseSVMformat(self.ranking, self.weight, skipComment = skipComment) + "\n")
    
    def ToBBR(self, fileName, threshold):
        with open(fileName, "w") as output:
            for obj in self.objects:
                 output.write(obj.ToBBRFormat(threshold) + "\n")
    
    def ToTreenet(self, fileName):
        with open(fileName, "w") as output:
            for obj in self.objects:
                output.write(obj.ToTreenetFormat() + "\n")
                
    def ToMatrixnetUniq(self, fileName, maxobjs = 10):
        oldid = -1
        with open(fileName, "w") as output:
            for obj in self.objects:
                if obj.id != oldid:
                    oldid = obj.id
                    n = 0
                obj.id = obj.id * maxobjs + n
                output.write(obj.ToTreenetFormat() + "\n")
                n += 1
        
    def IndexBlindFile(self, blindFileName):
        match_hash = {}
        with open(blindFileName, "r") as file:
            res = 0
            for line in file:
                fields = line.split("\t")
                if (len(fields) < 20):
                    continue;
                
                query = fields[0]
                
                if (fields[7] != 'UNKNOWN_ORIGINAL_QUERY'):
                    query = fields[7]
                
                url = fields[2]
                if (fields[8] != 'http://UNKNOWN_ORIGINAL_URL'):
                    url = fields[8]
                
                qdpair_hash = CalculateQDPairHash(query, url)
                first_candidate_scoords_list = fields[12]
                second_candidate_scoords_list = fields[16]
                scoords = first_candidate_scoords_list + ";" + second_candidate_scoords_list
                
                if not qdpair_hash in match_hash:
                    match_hash[qdpair_hash] = {}
                    
                if not scoords in match_hash[qdpair_hash]:
                    match_hash[qdpair_hash][scoords] = {}
                    match_hash[qdpair_hash][scoords]["BETTER"] = 0
                    match_hash[qdpair_hash][scoords]["EQUALLY"] = 0
                    match_hash[qdpair_hash][scoords]["WORSE"] = 0
                    
                match_hash[qdpair_hash][scoords][fields[17]] += 1
                
                
        return match_hash
        
    def ToMatrixnetPairs(self, fileName, blindFileName = ""):
        ids = {}
        match_hash = {}
        if len(blindFileName) > 0:
            match_hash = self.IndexBlindFile(blindFileName)
            
        for n in xrange(len(self.objects)):
            id = self.objects[n].id
            if id not in ids:
                ids[id] = []
            ids[id].append((n, self.objects[n].target))
        
        with open(fileName, 'w') as output:
            for group in ids.itervalues():
                for i in range(len(group)):
                    for j in range(len(group)):
                        if (i == j):
                            continue
                        if len(match_hash) > 0:
                            qdpair_hash = self.objects[group[i][0]].qdpair_hash
                            scoords = self.objects[group[i][0]].scoords + ";" + self.objects[group[j][0]].scoords
                            if qdpair_hash in match_hash and scoords in match_hash[qdpair_hash]:
                                for k in range(match_hash[qdpair_hash][scoords]["WORSE"]):
                                    print >> output, str(group[j][0]) + '\t' + str(group[i][0])
                                for k in range(match_hash[qdpair_hash][scoords]["EQUALLY"]):
                                    print >> output, str(group[i][0]) + '\t' + str(group[j][0]) + '\t' + str(0.5)
                                    print >> output, str(group[j][0]) + '\t' + str(group[i][0]) + '\t' + str(0.5)
                                for k in range(match_hash[qdpair_hash][scoords]["BETTER"]):
                                    print >> output, str(group[i][0]) + '\t' + str(group[j][0])
                        elif group[i][1] > group[j][1]:
                            print >> output, str(group[i][0]) + '\t' + str(group[j][0])
    
    def GetFeatureMeans(self):
        self.means = {}
        for obj in self.objects:
            for fname in obj.features.keys():
                if fname not in self.means:
                    self.means[fname] = 0
                self.means[fname] += obj.features[fname] / len(self.objects)
        return self.means
    
    def GetFeatureMin(self):
        featureMin = {}
        for obj in self.objects:
            for fname in obj.features.keys():
                if fname not in featureMin:
                    featureMin[fname] = obj.features[fname]
                featureMin[fname] = min(featureMin[fname], obj.features[fname])
        return featureMin

    def GetFeatureMax(self):
        featureMax = {}
        for obj in self.objects:
            for fname in obj.features.keys():
                if fname not in featureMax:
                    featureMax[fname] = obj.features[fname]
                featureMax[fname] = max(featureMax[fname], obj.features[fname])
        return featureMax
        
    def GetFeatureDispersions(self):
        self.GetFeatureMeans()
        self.disps = {}
        for obj in self.objects:
            for fname in obj.features.keys():
                if fname not in self.disps:
                    self.disps[fname] = 0
                self.disps[fname] += (obj.features[fname] - self.means[fname])**2
        for fname in self.disps.keys():
            self.disps[fname] = self.disps[fname] / (len(self.objects) - 1);
        return self.disps
    
    def Normalize(self): 
        self.GetFeatureMeans()
        self.GetFeatureDispersions()       
        for obj in self.objects:
            for fname in obj.features.keys():
                if self.disps[fname] != 0:
                    obj.features[fname] = (obj.features[fname] - self.means[fname] ) / sqrt(self.disps[fname])
                else:
                    obj.features[fname] = (obj.features[fname] - self.means[fname])
    
    def ScaleFeatures(self, lower, upper):
        if lower >= upper:
            raise Exception, "Lower boundary must be less than upper boundary"
            
        self.lowerFeatureBoundary = lower
        self.upperFeatureBoundary = upper
        
        self.featureMin = self.GetFeatureMin()
        self.featureMax = self.GetFeatureMax()
        
        for obj in self.objects:
            for fname in obj.features.keys():
                 if self.featureMin[fname] == self.featureMax[fname]:
                    continue
                 if obj.features[fname] == self.featureMin[fname]:
                    obj.features[fname] = lower
                 elif obj.features[fname] == self.featureMax[fname]:
                    obj.features[fname] = upper
                 else:
                    obj.features[fname] = lower + (upper - lower)*(obj.features[fname] - self.featureMin[fname]) / (self.featureMax[fname] - self.featureMin[fname])
                    
    def GetTargetValueProperties(self):
        targetmean = 0
        targetdisp = 0
        for obj in self.objects:
            targetmean += obj.target / len(self.objects)
        
        for obj in self.objects:
            targetdisp += ((obj.target - targetmean)**2) / (len(self.objects) - 1)
            
        return [targetmean, targetdisp]
    
    def AddObject(self, obj):
        if isinstance(obj, LearningObject):
            self.objects.append(obj)
        else:
            raise TypeError, "The object of incorrect type was given"
        
    def LoadDataFromFile(self, fileName, separator="\t", idColumn = -1, targetValueColumn=0, featuresColumns=[], commentColumn = -1):
        input = open(fileName, "r")
        
        objindex = 1
        for strLine in input:
            elements = strLine.strip().split(separator)
            obj = LearningObject()
            
            if round(float(elements[targetValueColumn])) == float(elements[targetValueColumn]):
                obj.target = int(float(elements[targetValueColumn]))
            else:
                obj.target = float(elements[targetValueColumn])
            
            if commentColumn > -1:
                obj.comment = elements[commentColumn]
            
            if idColumn > -1:
                obj.id = elements[idColumn]
            else:
                obj.id = objindex
            
            objindex += 1
            
            counter = 1
            for feat in featuresColumns:
                obj.features[counter] = float(elements[feat])
                counter += 1
            
            self.objects.append(obj)
        
        input.close()
        
    def Merge(self, ld):
       if not isinstance(ld, LearningData):
          raise TypeError, "LearningData object is expected"
            
       if len(ld.objects) != len(self.objects):
          raise Exception, "The number of objects differs"
        
       if len(self.objects[0].features.keys()) > 0:
           lastFeatureIndex = self.objects[0].features.keys()[len(self.objects[0].features)-1] + 1
       else:
           lastFeatureIndex = 0
       for i in range(0, len(self.objects)):
           counter = lastFeatureIndex
           for feature in ld.objects[i].features.keys():
               self.objects[i].features[counter] = ld.objects[i].features[feature]
               counter += 1

    def IsRanking(self):
        return self.ranking

    def ConvertRankingToPairClassification(self):
        if not self.IsRanking():
            return
        self.ranking = False
        new_objects = []
        for i in range(0, len(self.objects)):
            for j in range(i+1, len(self.objects)):
                if self.objects[i].id != self.objects[j].id:
                    continue
                    
                if self.objects[i].target != self.objects[j].target:
                    new_obj = LearningObject()
                    new_obj.id = int(str(self.objects[i].id) + str(self.objects[j].id))
                    new_obj.comment = str(self.objects[i].comment) + ' ' + str(self.objects[j].comment)
                    new_obj.target = 1 if self.objects[i].target > self.objects[j].target else -1
                    #numfeats = len(self.objects[i].features)
                    for fname in self.objects[i].features:
                        new_obj.features[fname] = self.objects[i].features[fname] - self.objects[j].features[fname]
                    #    new_obj.features[fname] = self.objects[i].features[fname]
                    #    new_obj.features[fname + numfeats] = self.objects[j].features[fname]
                    new_objects.append(new_obj)
        self.objects = new_objects
    
    def ConvertClassificationToOptimization(self, pos_weights = False):
        for obj in self.objects:
           if obj.target == 0:
             print "Cannot convert to optimization for transductive data sets."
           elif obj.target == -1:
              obj.target = 1
              for f in obj.features.keys():
                 obj.features[f] = -1*obj.features[f]
        
        self.weight = True
        if pos_weights:
           obj_count = len(self.objects)
           for f in self.objects[0].features.keys():
              obj = LearningObject()
              obj.target = 0
              obj.weight = (obj_count + len(self.objects[0].features.keys()))
              for fname in self.objects[0].features.keys():
                 obj.features[fname] = 1 if f == fname else 0
              self.objects.append(obj)
    
    def ShuffleObjects(self):
        shuffle(self.objects)

    def Split(self, proportion):
        newData = LearningData()
        for obj in self.objects[int(round(len(self.objects) * proportion)):]:
            newData.AddObject(obj)
        del self.objects[int(round(len(self.objects) * proportion)) : len(self.objects)]
        return newData
      
    def FilterData(self, featureName, filterbytarget = False, upthreshold = maxint, downthreshold = -maxint-1):
        filtered_objs = []
        for obj in self.objects:
            if filterbytarget:
                val = obj.target
            else:
                val = obj.features[featureName]

            if val < upthreshold and val > downthreshold:
                    filtered_objs.append(obj)

        self.objects = filtered_objs
    
    def RemoveFeature(self, featureName):
        for obj in self.objects:
            del obj.features[featureName]
            
    def GetObjects(self):
        return self.objects
   
class SVMModelBase:
    def CalculateAngleBetweenHyperplane(self, model):
        coefs1 = self.GetLinearKernelCoefs()
        coefs2 = model.GetLinearKernelCoefs()
        if isinstance(coefs1, dict):
            coefs1 = self._ConvertCoefsToList_(coefs1)
        if isinstance(coefs2, dict):
            coefs2 = self._ConvertCoefsToList_(coefs2)
        if not len(coefs1) == len(coefs2):
            raise Exception, "Models should have the same factor number"
        value = 0
        for index in range(len(coefs1)):
            value += coefs1[index]*coefs2[index]
        norm1 = 0
        norm2 = 0
        for index in range(len(coefs1)):
            norm1 += math.pow(coefs1[index], 2)
            norm2 += math.pow(coefs2[index], 2)
        norm1 = math.sqrt(norm1)
        norm2 = math.sqrt(norm2)
        return 180*math.acos(value/(norm1*norm2))/math.pi
        
    def _ConvertCoefsToList_(self, coefs_dict):
        coefs_list = []
        for index in range(len(coefs_dict)):
            coefs_list.append(coefs_dict[index + 1])
        return coefs_list
           
class SVMModel(SVMModelBase):
    def __init__(self, modelFileName):
        self.supportVectors = []
        self.kernelParameters = []
        
        modelFile = open(modelFileName, "r")
        modelFile.readline() # read header
        self.kernelType = int(modelFile.readline().split("#")[0].strip()) # read kernel type
        dParam = modelFile.readline().split("#")[0].strip() # d parameter
        gParam = modelFile.readline().split("#")[0].strip() # g parameter
        sParam = modelFile.readline().split("#")[0].strip() # s parameter
        rParam = modelFile.readline().split("#")[0].strip() # r parameter
        uParam = modelFile.readline().split("#")[0].strip() # u parameter
        featuresCount = modelFile.readline().split("#")[0].strip() # Features count
        learningDocs = modelFile.readline().split("#")[0].strip() # learning documents
        supportVectorsCount = modelFile.readline().split("#")[0].strip() # support vectors
        b = modelFile.readline().split("#")[0].strip() # b - hyperplane bias
   
        for i in range(1,int(supportVectorsCount)):
           sVect = modelFile.readline()
           sVect = sVect.strip().split("#")[0].strip().split(" ")
           supportVector = [float(sVect[0]), {}]
           for i in range(0,int(featuresCount)):
               supportVector[1][int(sVect[1 + i].split(":")[0])] = float(sVect[1 + i].split(":")[1])
           self.supportVectors.append(supportVector)
        
        modelFile.close()
        self.kernelParameters = [float(dParam), float(gParam), float(sParam), float(rParam), uParam] 
    
    def GetLinearKernelCoefs(self):
        if self.kernelType != 0:
            raise Exception, "Model is of non-linear kernel"
        
        weight = {}
        for sVec in self.supportVectors:
           features = sVec[1]
           alpha = sVec[0]
           for fname in features.keys():
               value = features[fname]
               if fname not in weight:
                    weight[fname] = 0
               weight[fname] += float(alpha) * float(value)
    
        return weight
    
    def innerProduct(self, vector1, vector2):
       res = 0
       for fname in vector1.keys():
          res += vector1[fname]*vector2[fname]
       return res   
    
    def getSquareNorm(self, vector):
        res = 0
        for fname in vector.keys():
            res += vector[fname] ** 2
        return res
    
    def diffVector(self, vector1, vector2):
        res = []
        for fname in set(vector1.keys().extend(vector2.keys())):
            res.append(vector1[fname] - vector2[fname])
        return res     
    
    def getValue(self, vector):
       # Polynomial kernel
        res = 0
        if self.kernelType == 1:
            s = self.kernelParameters[2]
            d = self.kernelParameters[0]
            c = self.kernelParameters[3]
            for supportVector in self.supportVectors:
                res += supportVector[0]*((s * self.innerProduct(vector, supportVector[1]) + c) ** d)

        elif self.kernelType == 2:
            gamma = self.kernelParameters[1]
            res = 0
            for supportVector in self.supportVectors:
                res += supportVector[0]*exp(-gamma * self.getSquareNorm(self.diffVector(vector, supportVector[1])))
        else:
            raise Exception, "Unknown kernel type"
        return res
           
    def ConvertToCpp(self, fileName):
        
        file = open(fileName, "w")
        print >> file, "byte kernelType = ", self.kernelType, ";"
        print >> file, "double dParam = ", self.kernelParameters[0], ";"
        print >> file, "double gParam = ", self.kernelParameters[1], ";"
        print >> file, "double sParam = ", self.kernelParameters[2], ";"
        print >> file, "double rParam = ", self.kernelParameters[3], ";\n"

        print >> file, "TVector< TVector<double> > supportVectors;"
        print >> file, "TVector<double> supportVectorsWeights;"        
        
        for sv in self.supportVectors:
            print >> file, "supportVectorsWeights.push_back(", sv[0], ");"
            
            print >> file, "vector<double> feats;"
            
            for feature in sv[1].values():
                print >> file, "feats.push_back(", feature, ");"
            
            print >> file, "supportVectors.push_back(feats);"
        
        file.close()
    
# !!! support only linear kernel        
class LibSVMModel(SVMModelBase):
    def __init__(self, modelFileName):
        self.coefs = []
        modelFile = open(modelFileName, "r")
        # read header
        for id in range(6):
            modelFile.readline()
        
        for line in modelFile:
            self.coefs.append(float(line.strip()))
        modelFile.close()
                
    def GetLinearKernelCoefs(self):
        kernel_coefs = {}
        for index in range(len(self.coefs)):
           kernel_coefs[index + 1]  = self.coefs[index]
        return kernel_coefs


if __name__ == "__main__":
    from sys import argv
    learner = MatrixNETLearner(argv[1])
    learner.learn_type = 'pfound'
    learner.options['iterations'] = 20
    learner.CrossValidation(argv[2], 3, True)

