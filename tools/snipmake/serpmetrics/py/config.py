#!/usr/bin/python
# -*- coding: utf-8 -*-

######### base configuration params ###########
data_folder     = "./data/"
metrics         = "./snipmetricsapp"
wizards_py      = "./wizards/wizards.py"
stopwords       = "./language/stopword.lst"
porno_config    = "./pornofilter/porno_config.dat"
wizards_url     = "xmlsearch.hamster.yandex.ru"
serp_type       = {"country":"RU"}
streams = [
    {"name":"Y.WEB_RU_MIXED_VALIDATE_DEDUPLICATED", "file":"ya_mixed_validate_deduplicated", "sysId":1457},
    {"name":"G.WEB_RU_MIXED_VALIDATE_DEDUPLICATED", "file":"gg_mixed_validate_deduplicated", "sysId":1459},
]
plot_name       = "Автометрики - RU"
plot_type       = "ru"
period          = 60
external_source = "http://sitelinks.i-folb.fog.yandex.net/gtametrics/view.py"
threads         = 4
razladki_streams = dict()
razladki_metrics_pref = "metrics"
razladki_url    = "http://launcher.razladki.yandex-team.ru/save_new_data/snippets"
###############################################

import optparse, sys, os
parser = optparse.OptionParser()
parser.add_option("-c", "--config", dest="cfg", help="config file path")
parser.add_option("-s", "--serp", dest="serp", help="simple serp file")
parser.add_option("-j", "--threads", dest="threads", help="run in N threads")
(options, args) = parser.parse_args()

print "Loading config: %s" % options.cfg
syspath = list(sys.path)
try:
    path, name = os.path.split(options.cfg)
    sys.path.append(path)
    exec "from " + name.split(".")[0] + " import *"
except:
    print "Bad config. Using default settings."
    pass
finally:
    sys.path = syspath
if options.threads:
    threads = int(options.threads)
log = os.path.join(data_folder, "output.log")
