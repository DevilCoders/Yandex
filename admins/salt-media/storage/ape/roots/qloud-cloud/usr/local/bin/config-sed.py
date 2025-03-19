#!/usr/bin/env python
import re
import sys

#this script make sed 's/sys.argv[1]/`cat sys.argv[2]`/g' in file sys.argv[3]
config = open(sys.argv[3],"r").read()
insert = open(sys.argv[2],"r").read()
result = re.sub(sys.argv[1], insert, config)
print "Config sed script create config " + sys.argv[3] + " :\n"
print result
config = open(sys.argv[3],"w")
config.write(str(result))
config.close()

