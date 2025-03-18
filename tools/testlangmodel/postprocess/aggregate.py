import sys
import math
from collections import defaultdict

stattable = None
wordcount = 0

notch = 0.1
blur = 1.0

def dumpstats():
    print "Dumping stats for wordcount", wordcount
    if not stattable:
        return
    scorekeys = stattable.keys()
    if len(scorekeys) <= 10:
        return
   
    print "Stats:", len(stattable)
    
    startpoint = math.floor(min(scorekeys) - 3 * blur)
    endpoint = math.ceil(max(scorekeys) + 3 * blur)
    weights = {}

    point = startpoint
    while point <= endpoint:
        weight = 0
        for key, freq in stattable.items():    
            delta = math.fabs(point - key) / blur
            weight += freq * math.exp(- delta * delta / 2)
        weights[point] = weight
        point += notch
    
    norm = sum(weights.values())     
    weights = weights.items()
    weights.sort(key = lambda x: x[0])

    outfile = file("%s.%d.csv" % (sys.argv[2], wordcount), "w")        
    for point, weight in weights:
        print >> outfile, point, ";",  1000 * weight / norm
    

for line in file(sys.argv[1], "r"):
    tokens = line.split()
    if wordcount != int(tokens[0]):
        dumpstats()            
        stattable = defaultdict(int)
        wordcount = int(tokens[0])
    
    score = float(tokens[1])
    stattable[score] += 1
dumpstats()
    
    
    
    
    
    
    
