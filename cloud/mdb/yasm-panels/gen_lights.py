#!/usr/bin/env python

from __future__ import absolute_import, print_function, unicode_literals

import json
import sys
from copy import deepcopy


def buildLightsPanel(maxCols, groupList, lightsRow):
    newDict = {'lights': [],
               'size': 150,
               'maxColsCount': maxCols,
               'mode': 'mini',
               'rows': 100}
    if len(groupList) > 32:
        actRows = int(len(groupList) / maxCols)
        if len(groupList) % maxCols != 0:
            actRows += 1
    else:
        actRows = 1
    for i in range(actRows):
        for j in lightsRow:
            newDict['lights'].append([])
    for group in groupList:
        try:
            grName, dbName = group.split(':')
        except Exception:
            grName = group
            dbName = 'total'
        if '-' in grName:
            shortGrName = '-'.join(grName.split('-')[1:])
        else:
            shortGrName = grName
        newG = deepcopy(lightsRow)
        for col in newG:
            for key in col:
                col[key] = col[key].replace('%SHORT_NAME%', shortGrName)
                col[key] = col[key].replace('%NAME%', grName)
                col[key] = col[key].replace('%DB%', dbName)
        for i in range(len(newG)):
            if len(groupList) > maxCols:
                insIndex = (groupList.index(group) // maxCols) * len(newG) + i
            else:
                insIndex = i
            newDict['lights'][insIndex].append(newG[i])
    return newDict


if __name__ == "__main__":
    if len(sys.argv) != 6:
        print("Usage: " + sys.argv[0] + " <max columns> <name> <groups> " +
              "<base template path> <html template path>")
        sys.exit(1)

    groupList = sys.argv[3].split(',')

    with open(sys.argv[4]) as templateFile:
        baseDict = json.loads('\n'.join(templateFile.readlines()))

    panel = json.dumps(buildLightsPanel(int(sys.argv[1]), groupList, baseDict),
                       indent=4, separators=(',', ': '))

    with open(sys.argv[5]) as htmlTemplate:
        lines = htmlTemplate.readlines()

    for line in lines:
        print(line.replace('%PANELCONFIG%', panel).replace('%PANELNAME%',
                                                           sys.argv[2]),
              end=' ')
