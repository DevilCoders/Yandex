#!/usr/bin/env python

from tornado import gen
from tornado import ioloop
from cocaine.services import Service

@gen.coroutine
def main():
    locator = Service("locator")
    try:
        yield locator.connect()
    except:
        print "1; error while connect to locator"
        exit(1)
    try:
        chan = yield locator.resolve("graphite")
        result = yield chan.rx.get()
    except:
        print "2; error while resolv service graphite"
        exit(1)
    print "0;Ok"

ioloop.IOLoop.current().run_sync(main, timeout=30)
