#!/usr/bin/env python

# Compares files produced by antirobot_evlogdump
# Factors should be generated from the same single access.log file

import sys
import re
import getopt;

def PrintUsage():
    print "Usage: factors-diff.py [-i <file with ignored factors>] factors1 factors2"

factors_re = re.compile("(?:[\w\|]+=[\d\.]+)(?:\ [\w\|]+=[\d\.]+)+")

File1 = None
File2 = None

ExitResult = 0

class IgnoredFactors:
    def __init__(self, fileName):
        if fileName:
            self.factors = set(filter(lambda x : x, [x.strip() for x in open(fileName).read().split('\n')]));
        else:
            self.factors = set();

    def FactorIgnored(self, factorName):
        return factorName.split('^')[0].split('|')[0] in self.factors;

def CompareLines(l1, l2, line_num, ignoredFactors):
    fields1 = l1.split('\t')
    fields2 = l2.split('\t')

    if len(fields1) != len(fields2):
        print "Field count is different in line %d" % line_num
        sys.exit(1)

    field_count = len(fields1)

    for i in xrange(field_count):
        if fields1[i] != fields2[i]:
            if factors_re.match(fields1[i]) and factors_re.match(fields2[i]):
                CompareFactors(fields1[i], fields2[i], line_num, ignoredFactors)

def CompareFactors(line1, line2, line_num, ignoredFactors):
    global ExitResult

    factors1 = dict((i.split('=')) for i in line1.split())
    factors2 = dict((i.split('=')) for i in line2.split())
    factors_keys = [i.split('=')[0] for i in line1.split()]
     
    diff1 = []
    diff2 = []

    for f1_key in factors_keys:
        if ignoredFactors.FactorIgnored(f1_key):
            continue;

#        if f1_key.find('^') >=0:
#            continue
        f1_value = factors1.get(f1_key)
        f2_value = factors2.get(f1_key)
        if f1_value != f2_value:
            diff1.append((f1_key, f1_value))
            diff2.append((f1_key, f2_value))
#factors1.pop(f1_key)
#factors2.pop(f1_key, None)


    if len(diff1) or len(diff2):
        PrintHeaderIfNeed()
        
        ExitResult = 1

        print 'line %d' % line_num
        print '-',
        for k,v in diff1:
            print '%s=%s' % (k,v), '\t',
        print 


        print '+',
        for k,v in diff2:
            print '%s=%s' % (k,v), '\t',
        print 
    


flag = False
def PrintHeaderIfNeed():
    global flag 

    if not flag:
        print '- %s' % File1
        print '+ %s' % File2
        flag = True

def Main():
    (options, args) = getopt.getopt(sys.argv[1:], "i:");

    options = dict(options);
    ignoredFactors = IgnoredFactors(options.get('-i'));
        
    global File1
    global File2

    if len(args) < 2:
        PrintUsage()
        sys.exit(2)

    File1 = args[0]
    File2 = args[1]

    f1 = open(File1)
    f2 = open(File2)

    line_num = 0

    while True:
        line_num += 1

        l1 = f1.readline()
        l2 = f2.readline()
        
        if l1 == '' and l2 == '':
            break
        elif l1 == '' and l2 != '':
            print "'%s' is less than '%s'" % (File1, File2)
        elif l2 == '' and l1 != '':
            print "'%s' is more than '%s'" % (File1, File2)

        CompareLines(l1, l2, line_num, ignoredFactors)


if __name__ == "__main__":
    Main()
    sys.exit(ExitResult)
