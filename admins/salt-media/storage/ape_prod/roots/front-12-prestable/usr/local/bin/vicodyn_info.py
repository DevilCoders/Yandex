#!/usr/bin/env python
from random import randint
from time import sleep

from tornado import gen, ioloop

from cocaine.services import Service

sleep(randint(1,10))

@gen.coroutine
def main():
    vicodyn = Service("vicodyn")
    chan = yield vicodyn.info()
    info = yield chan.rx.get()
    warning = '1; apps banned more then 50%: '
    error = '2; apps banned more then 80%: '
    for i in info['apps']:
        if info['apps'][i]['total']['peers'] != 0:
            perc = float(info['apps'][i]['total']['banned'])/float(info['apps'][i]['total']['peers'])*100
            if perc > 50:
                warning = warning + i + ','
            elif perc > 80:
                error = error + i + ','
    if error != '2; apps banned more then 80%: ':
        print (error)
    elif warning != '1; apps banned more then 50%: ':
        print (warning)
    else:
        print '0;Ok'

try:
    vicodyn = ioloop.IOLoop.current().run_sync(main, timeout=15)
except ioloop.TimeoutError:
    print '1; timeout'
except Exception as e:
    print '1: {}'.format(e)
