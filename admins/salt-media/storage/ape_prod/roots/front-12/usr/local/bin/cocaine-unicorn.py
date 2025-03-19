#!/usr/bin/env python

import os
import sys
import time
from tornado import gen
from tornado import ioloop
from cocaine.services import Service
from random import randint
from time import sleep
from tornado.locks import Event

sleep(randint(1,10))
event = Event()

CRITSTATE='2'
WARNSTATE='1'
ACRITSTATE=CRITSTATE

TIMEOUT=30
STATECHECKFILE=''
if len(sys.argv) > 1:
    TIMEOUT=int(sys.argv[1])
if len(sys.argv) > 2:
    STATECHECKFILE=str(sys.argv[2])

if len(STATECHECKFILE) > 2:
    try:
         cftime=int(os.path.getmtime(STATECHECKFILE))
         ctime=int(time.time())
         if (ctime-cftime) < 3601:
             ACRITSTATE=WARNSTATE
    except:
        pass


@gen.coroutine
def main(node,unicorn):
    yield event.wait()
    error = 0
    chan = yield unicorn.create(node,"")
    try:
        result = yield chan.rx.get()
        yield chan.tx.close()
    except:
        error = 1
    try:
        chan = yield unicorn.get(node)
        result = yield chan.rx.get()
        yield chan.tx.close()
    except:
        error = 2
    try:
        chan = yield unicorn.put(node,"pidor",result[1])
        result = yield chan.rx.get()
        yield chan.tx.close()
    except:
        error = 3
    try:
        chan = yield unicorn.remove(node,result[1][1])
        result = yield chan.rx.get()
        yield chan.tx.close()
    except:
        error = 4
    if error == 0:
        print("0; Ok")
    elif error == 1:
        print("1; node exist")
    elif error == 2:
        print("%s; get method failed" % ACRITSTATE)
        exit(0)
    elif error == 3:
        print("%s; put method failed" % ACRITSTATE)
    elif error == 4:
        print("%s; remove method failed" % ACRITSTATE)
        exit(0)

@gen.coroutine
def subscribe(node,unicorn):
    chan = yield unicorn.subscribe(node,"")
    event.set()
    try:
        result = yield chan.rx.get()
        result = yield chan.rx.get()
        result = yield chan.rx.get()
    except:
        print("%s; subscribe failed" % ACRITSTATE)
        exit(0)
    yield chan.tx.close()

node = "/unicorn_test/" + str(randint(0,100500))
unicorn = Service("unicorn")
try:
    ioloop.IOLoop.current().run_sync(lambda: [main(node,unicorn), subscribe(node,unicorn)], timeout=TIMEOUT)
except Exception as e:
    print("%s; subscribe failed with error %s" % (ACRITSTATE, e))
    exit(0)

