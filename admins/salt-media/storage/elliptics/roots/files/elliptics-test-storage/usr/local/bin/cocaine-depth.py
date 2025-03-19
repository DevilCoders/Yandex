#!/usr/bin/env python

from tornado import gen
from tornado import ioloop
from cocaine.services import Service

@gen.coroutine
def main():
    warning = "1; app status broken: "
    error = "2; depth is full: "
    node = Service("node",endpoints=(('::1', 10061),))
    try:
        chan = yield node.list()
    except:
        print "1; error while connect to service node"
        exit(0)
    app_list = yield chan.rx.get()
    for name in app_list:
        app = Service(name,endpoints=(('::1', 10061),))
        try:
            chan = yield app.info()
            info = yield chan.rx.get()
            if info["queue"]["depth"] == info["queue"]["capacity"]:
                if name != "v012-karma":
                    error = error + name + ","
        except:
            warning = warning + name + ","
    if error != "2; depth is full: ":
        print (error)
    elif warning != "1; app status broken: ":
        print (warning)
    else:
        print ("0;Ok")

ioloop.IOLoop.current().run_sync(main, timeout=30)

