#!/usr/local/bin/python
# -*- coding: utf8 -*-

def readConfig(configname):
    config = {}
    from string import strip
#    from re import split as splitre
    with open(configname, "r") as configfile:
      for line in configfile:
        if line[0] == '#' or len(line.strip()) == 0:
            continue
#        print splitre('[ :=]*|\n', line)
        key, value = map(strip, line.split('=', 1))
        if value.find('|') != -1:
            value = map(strip, value.split('|'))
        config[key] = value
    return config

    
def makeHeader(features, featnames, outfile):
    with open(outfile, "w") as formula:
        for name in featnames:
            formula.write('     /*' + name + '*/    %.6f,\n' % (features[name] if name in features else 0.0))
