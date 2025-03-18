#!/usr/local/bin/python
# -*- coding: utf8 -*-
from sys import argv, path
from os.path import dirname
path.insert(0, dirname(dirname(__file__)))
import random
from candidate_parser import SnippetsData
from common import readConfig

def selectCandidates(candfilename, candidates_proportion, algorithms, electfilename):
    '''
    divide tasks by <algorithms> (one task - one algorithm)
    select randomly <candidates_proportion> candidates from <candfilename> for each task
    write result to <electfilename>
    '''
    candfile = open(candfilename, "r")
    electfile = open(electfilename, "w")
    #top_5, first_half, second_half = candidates_proportion
    
    random.seed()
    '''
    algo = algorithms.keys()[0]
    done_id = -1
    '''
    candline = candfile.readline()
    candidate = SnippetsData(candline)
    while candline != '':
        candidates = []
        c_id = candidate.id
        c_algo = candidate.algorithm
        while c_id == candidate.id and c_algo == candidate.algorithm:
            candidates.append(candline)
            candline = candfile.readline()
            candidate.LoadSimple(candline)
        '''
        if c_id == done_id or c_algo != algo:
            continue
        
        j = len(candidates)
        if j == 1:
            continue
        
        if j <= sum(candidates_proportion):
            for i in range(0, j):
                electfile.write(candidates[i])
        '''
        if c_algo not in algorithms:
            continue
        if len(candidates) < algorithms[c_algo][0]:
            continue
        else:
            candidates_indexes = set()
            '''
            top5_range = range(0,min(5,j+1))
            first_half_range = range(0, max(j/2+1, top_5+first_half))
            second_half_range = range(j/2+1,j)
            '''
            
            item = 0
            for i in range(algorithms[c_algo][0]):
                candidates_indexes.add(i)
            for i in range(min(algorithms[c_algo][1], len(candidates) - algorithms[c_algo][0])):
                while item in candidates_indexes:
                    item = random.choice(range(len(candidates)))
                candidates_indexes.add(item)
            '''
            for i in range(0, top_5):
                while (item == -1) or item in candidates_indexes:
                    item = random.choice(list(set(top5_range)-candidates_indexes))
                candidates_indexes.add(item)
            for i in range(0, first_half):
                while item in candidates_indexes:
                    item = random.choice(list(set(first_half_range)-candidates_indexes))
                candidates_indexes.add(item)
            for i in range(0, second_half):
                while item in candidates_indexes:
                    item = random.choice(list(set(second_half_range)-candidates_indexes))
                candidates_indexes.add(item)
            '''
            for index in candidates_indexes:
                electfile.write(candidates[index])
        
        print c_id, c_algo
        '''
        algorithms[c_algo] -= 1
        if algorithms[c_algo] == 0:
            del algorithms[c_algo]
        if len(algorithms) == 0:
            break
        algo = random.choice(algorithms.keys())
        done_id = c_id
        '''
    candfile.close()
    electfile.close()

if __name__ == "__main__":
    '''
    usage: selection.py <all candidates file> <resultfile>
    '''
    #config = readConfig(argv[1])
    candfile = argv[1]
    electfile = argv[2]
    #candidates_count = int(config['algo_candidates_count'])
    #snippets_to_assess = int(argv[4])
    
    #algo_proportion = map(lambda x: int(float(x)/100*snippets_to_assess), config['algorithm_proportion'])
    #algorithms = {'Algo1':algo_proportion[0], 'Algo2':algo_proportion[1], 'Algo3_pairs':algo_proportion[2]}
    algorithms = {'Algo2':(3,2), 'Algo3_pairs':(3,2)}
    
    #ATTENTION! algorithm and candidates proportions from config aren't used
    selectCandidates(candfile, None, algorithms, electfile)
