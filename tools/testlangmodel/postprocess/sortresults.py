import sys

infile = file(sys.argv[1], "r")
outfile = file(sys.argv[2], "w")

recs = []
for line in infile:
    tokens = line.split("\t")
    if len(tokens) != 3: continue
    if int(tokens[0]) == 0: continue
    recs.append( (int(tokens[0]), float(tokens[1]), tokens[2].strip()) )
    
    
recs.sort(key = lambda x: x[2])
recs.sort(key = lambda x: x[1])
recs.sort(key = lambda x: x[0])

for rec in recs:
    print >> outfile, "%d\t%f\t%s" % rec

    
