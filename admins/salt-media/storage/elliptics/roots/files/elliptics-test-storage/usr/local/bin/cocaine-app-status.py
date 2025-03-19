#!/usr/bin/env python
import sys
from tornado import gen
from tornado import ioloop
from cocaine.services import Service

@gen.coroutine
def main():
    ENDPOINTS = [('::1', sys.argv[1])]
    node = Service("node",endpoints=ENDPOINTS)
    try:
        chan = yield node.list()
    except:
        print "2; error while connect to service node"
        exit(0)

    app_broken = ""
    app_good = ""
    app_list = yield chan.rx.get()
    for app_name in app_list:
        app_status = yield node.info(app_name,0x04|0x01)
        app_info = yield app_status.rx.get()
        try:
            depth =  app_info["queue"]["depth"]
            capacity = app_info["queue"]["capacity"]
            if capacity == 0:
                continue
            state = app_info["state"]
        except KeyError:
            print "2; %s app is not running" %sys.argv[1]
            exit(1)

        if state == "broken":
            app_broken += "{0}: broken, ".format(app_name)
        elif float(depth)/capacity > 0.9:
            app_broken += "{0} queue is full, ".format(app_name)
        else:
            app_good += "{0} good; ".format(app_name)

    if app_broken:
        app_broken = "2; "+app_broken
        print app_broken
    elif app_good:
        print "0; OK "+app_good

if __name__ == "__main__":
    ioloop.IOLoop.current().run_sync(main)
