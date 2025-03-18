# -*- coding: utf-8 -*-

from datetime import datetime
from options import get_option_value

def writeToLog(url, reqVariables = []):
    import traceback
    log = open(get_option_value("logDir") + "error_log.txt", "a")
    print >> log, "------------------- " + str(datetime.now()) + " ----------------------"
    print >> log, url
    if len(reqVariables):
        print >> log, "vars: "+"\t".join(reqVariables) + "\t"
    traceback.print_exc(file = log)
    log.close()


# These functions can be used for profiling
def profileStart(name):
    import hotshot, hotshot.stats
    print "Profile started"
    prof = hotshot.Profile(name + ".prof")
    prof.start()
    return prof

def profileEnd(prof, name):
    import hotshot, hotshot.stats
    prof.stop()
    prof.close()
    stats = hotshot.stats.load(name + ".prof")
    stats.sort_stats('time', 'calls')
    stats.print_stats(20)
