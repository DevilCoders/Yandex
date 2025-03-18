#!/usr/bin/env python

import os
import imp

if __name__ == "__main__":
    hookSourceFile  = os.path.join('..', '..', '..', '..', 'devtools', 'fleur', 'imports', 'installhook.py')
    imp.load_source('installhook', os.path.join(os.path.dirname(__file__), hookSourceFile))

import sys
from optparse import OptionParser

from analyzer import Analyzer
from antirobot.scripts.learn.make_learn_data.tweak import rnd_request


def OptParser():
    parser = OptionParser(
'''Usage: %prog cmd [options]
where cmd can be:
    prepare     long-initial running calculation
    final       load prepared data and print it
''')
    parser.add_option('', '--rndreq', dest='rndreqFile', action='store', type='string', help="path to rnd_reqdata")
    parser.add_option('', '--prepared', dest='prepared', action='store', type='string', help="path to prepared data")
    return parser

def Main():

    optParser = OptParser()
    (opts, args) = optParser.parse_args()

    if not args or not opts.rndreqFile or args[0] not in ['prepare', 'final']:
        optParser.print_usage()
        sys.exit(2)

    cmd = args[0]

    rndReqdataFile = opts.rndreqFile
    rndReqData = rnd_request.RndRequestData.Load(open(rndReqdataFile))
    outDir = opts.prepared if opts.prepared else '.'

    analyzer = Analyzer(rndReqData)
    analyzer.UpdateRndReqFull(rnd_request.GetRndReqFullIter(rndReqdataFile))

    analyzer.mapreduce_bin = 'mapreduce26'

    if cmd == 'prepare':
        analyzer.Trace("Preparing..")
        analyzer.Prepare()
        analyzer.SavePrepared(outDir)
        analyzer.Trace("All done.")
        analyzer.PrintStat()
    elif cmd == 'final':
        analyzer.LoadPrepared(outDir)
        analyzer.PrintStat()
    else:
        analyzer.Trace('Bad command')

if __name__ == "__main__":
    Main()
