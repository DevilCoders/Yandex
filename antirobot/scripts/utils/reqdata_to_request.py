#!/usr/bin/env python
# -*- coding: utf-8 -*-
'''Программа преобразует текстовое представление событий TRequestData для передачи
их программе antirobot/scripts/utils/send_fullreqs.py

Она предназначена для использования совместно с утилитой evlogdump для обработки
файлов, которые создаются "сохранятором" запросов (antirobot/daemon_lib/request_saver.h).
Допустим, у нас есть файл all_requests.log, созданный сохранятором, тогда для передачи 
всех запросов роботоловилке нужно выполнить последовательность команд

cat all_requests.log | antirobot_evlogdump --tr | reqdata_to_fullreq.py | send_fullreqs.py
'''

import sys
from optparse import OptionParser
from symbol import argument
from urllib import unquote_plus
import re

def ParseArgs():
    parser = OptionParser('Usage: %prog [input file]')
    parser.add_option('', '--rndreq', dest='rndReq', action='store_true', help="Assume rndreq file on input")
    return parser.parse_args()

timeRe = re.compile("^X-Start-Time: .*?\r\n", re.MULTILINE)
xffyRe = re.compile("^X-Forwarded-For-Y: .*?$", re.MULTILINE);
reqStrIndex = 13
def main():
    (opts, args) = ParseArgs()
    argument = args[0] if len(args) > 0 else None
    input = sys.stdin if not argument or argument == "-" else open(argument)

    i = 0
    try:
        for line in input:
            i += 1
            fields = line.rstrip().split('\t')
            if opts.rndReq:
                reqStr = fields[3].replace(r'\r\n', '\r\n')
            else:
                reqStr = unquote_plus(' '.join(fields[reqStrIndex:]))

            if not xffyRe.search(reqStr):
                continue
                
            print reqStr.strip() + '\r\n',
            if not timeRe.search(reqStr):
                print 'X-Start-Time: %s\r\n' % fields[0],
            print '\r\n\r\n',

    except:
        print >>sys.stderr, "Exception on line: %d" % i
        raise


if __name__ == "__main__":
    main()
