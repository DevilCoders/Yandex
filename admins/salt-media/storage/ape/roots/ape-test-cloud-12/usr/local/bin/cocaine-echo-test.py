#!/usr/bin/env python

import msgpack
from cocaine.services import Service

from tornado.gen import coroutine
from tornado.ioloop import IOLoop

ENDPOINTS = [("i-6dab261f659e.qloud-c.yandex.net", 10053)]

#service = Service('echo', endpoints=ENDPOINTS)
service = Service('echo')

@coroutine
def main():
    # open new stream
    channel = yield service.enqueue('http')
    # send a chunk of data to the worker handler 
    yield channel.tx.write('SomeMessage')
    # close the stream. To notify the worker, that we've finished our data stream.
    yield channel.tx.close()
    # read a reply
    chunk = yield channel.rx.get()
    print(msgpack.unpackb(chunk))

if __name__ == '__main__':
    IOLoop.current().run_sync(main, timeout=5)

