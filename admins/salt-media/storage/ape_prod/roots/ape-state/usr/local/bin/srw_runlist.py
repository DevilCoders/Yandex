#!/usr/bin/env python
import json
from tornado import gen
from tornado import ioloop
from cocaine.services import Service
from random import randint

@gen.coroutine
def main():
    unicorn_runlist = "/darkvoice/cfg/develop/runlist/production"
    unicorn_srw_runlist = "/darkvoice/cfg/develop/runlist/srw"
    unicorn = Service("unicorn", endpoints=[['dealer-12.ape.yandex.net', 10053]])
    chan_unicorn = yield unicorn.subscribe(unicorn_runlist)
    while True:
        result = yield chan_unicorn.rx.get()
        print 'get update'
        srw = {}
        for i in result[0]:
            if 'srw' in i or 'rbtorrent' in i or 'echo-deploy-test' in i:
                srw[i] = result[0][i]
        sle = randint(2,10)
        print 'sleep on {}'.format(sle)
        yield gen.sleep(sle)
        chan = yield unicorn.get(unicorn_srw_runlist)
        s = yield chan.rx.get()
        yield chan.tx.close()
        if srw != s[0]:
            chan = yield unicorn.put(unicorn_srw_runlist, srw, s[1])
            s = yield chan.rx.get()
            yield chan.tx.close()
            print 'update runlist'

ioloop.IOLoop.current().run_sync(main)
