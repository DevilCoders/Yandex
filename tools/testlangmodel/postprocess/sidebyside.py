import sys
import os.path

for i in range(100):
    path1 = "%s.%d.csv" % (sys.argv[1], i)
    path2 = "%s.%d.csv" % (sys.argv[2], i)
    
    if not os.path.exists(path1) or not os.path.exists(path2):
        continue
                
    graph1 = dict([map(float, x.split(';')) for x in file(path1, "r").readlines()])
    graph2 = dict([map(float, x.split(';')) for x in file(path2, "r").readlines()])
    
    outpath = "%s.%d.csv" % (sys.argv[3], i)
    print >> sys.stderr, outpath
    out = file (outpath, "w") 

    allkeys = list(set(graph1.keys() + graph2.keys()))
    allkeys.sort()
    
    total1 = 0.0
    total2 = 0.0
    
    print >> out, "Score;Quotes Density;Non-quotes Density;Quotes Distribution; Non-quotes Distribution; Quotes Share" 
    for key in allkeys:
        
        value1 = graph1.get(key, 0)
        value2 = graph2.get(key, 0)
        
        total1 += value1
        total2 += value2
        
        if value1 + value2 > 0:
            quote = value1 / (value1 + value2)
        else:
            quote = 0.5
        
        print >> out, "%f;%f;%f;%f;%f;%f" % (key, value1, value2, total1 / 1000, total2 / 1000, quote)
        
        
        
        




