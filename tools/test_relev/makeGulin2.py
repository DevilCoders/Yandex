#!/usr/bin/env python

import sys

print "words\turl\tscore\tdate"
for line in sys.stdin:
    print "%s\thttp://www.yandex.ru/\t30\t20080725" % line.strip()
