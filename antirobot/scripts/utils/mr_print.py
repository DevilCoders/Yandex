# -*- coding: utf-8 -*-

from devtools.fleur.util.mapreduce import MapReduce

import random

def GetProxy(mrServer):
    if mrServer.startswith('betula'):
        return 'tilia.yandex.ru:8081'
    elif mrServer.startswith('redwood'):
        return 'strobil.search.yandex.net:8081'
    elif mrServer.startswith('abies'):
        return 'larix03.yandex.ru:8081'
    elif mrServer.startswith('cedar'):
        return 'thuja.search.yandex.net:8081'
    elif mrServer.startswith('sakura'):
        return 'padus0%d.search.yandex.net:8081' % random.randrange(3, 10)
    else:
        raise Exception, "Unknown mapreduce server"


def PrintMrTable(tableName, outFile, server, usingSubkey = False, lstrip = False):
    # Параметр net_table=mrproxy задан на основе обсуждения
    # https://clubs.at.yandex-team.ru/stackoverflow/2732/2759#reply-stackoverflow-2759
    for rec in MapReduce.getSample(tableName, count=None, server=server, scheduleAttrs={"net_table" : "mrproxy"}):
        data = [str(rec.key)]
        if usingSubkey:
            data += [str(rec.subkey)]
        data += [str(rec.value)]

        if lstrip:
            data = [x.strip() for x in data]
        print >>outFile, '\t'.join(data)

