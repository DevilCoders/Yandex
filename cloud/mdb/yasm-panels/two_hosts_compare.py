#!/usr/bin/env python

import sys
import json
from copy import deepcopy


def buildComparePanel(baseDict, maxes, left, right):
    newDict = deepcopy(baseDict)
    newDict['charts'] = []
    for chart in baseDict['charts']:
        leftChart = deepcopy(chart)
        for signal in leftChart['signals']:
            signal['signal'] = signal['signal'].replace('%NAME%', left)
        leftChart['col'] = 0

        chartType = leftChart['title'].split()[0]

        if chartType in maxes:
            leftChart['maxValue'] = maxes[chartType]
        newDict['charts'].append(leftChart)

        rightChart = deepcopy(chart)
        for signal in rightChart['signals']:
            signal['signal'] = signal['signal'].replace('%NAME%', right)
        rightChart['col'] = 1

        if chartType in maxes:
            rightChart['maxValue'] = maxes[chartType]
        newDict['charts'].append(rightChart)

    return newDict

if __name__ == "__main__":
    if len(sys.argv) != 6:
        print("Usage: " + sys.argv[0] + " <base template path> " +
            "<html template path> <left host> <right host> <max tps>")
        sys.exit(1)

    with open(sys.argv[1]) as templateFile:
        baseDict = json.loads('\n'.join(templateFile.readlines()))

    maxes = {
        'iostat': 100.0,
        'netstat': 1024.0**3,
        'read-bytes': 5.0*10**8,
        'write-bytes': 5.0*10**8,
        'read-iops': 5.0*10**4,
        'write-iops': 5.0*10**4,
        'avg-query': 100.0,
        'conn': 2000.0
    }

    maxes['tps'] = float(sys.argv[5])

    panel = json.dumps(buildComparePanel(baseDict, maxes,
                                         sys.argv[3], sys.argv[4]),
                       indent=4, separators=(',', ': '))

    with open(sys.argv[2]) as htmlTemplate:
        lines = htmlTemplate.readlines()

    for line in lines:
        print(line.replace('%HOSTLEFT%', sys.argv[3]).replace('%HOSTRIGHT%',
                sys.argv[4]).replace('%PANELCONFIG%', panel).rstrip())
