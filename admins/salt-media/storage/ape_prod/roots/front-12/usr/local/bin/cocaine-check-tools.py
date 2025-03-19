#!/usr/bin/env python
from tornado import gen
from tornado import ioloop
from cocaine.services import Service
from cocaine.exceptions import ServiceError
import time
import socket
import argparse

timeout = 3
node = "/depth_check/downtime"
unicorn = Service("unicorn")

@gen.coroutine
def main():
    #create main node if exist
    try:
        chan_unicorn = yield unicorn.get(node,"")
        result_list = yield chan_unicorn.rx.get(timeout=timeout)
        yield chan_unicorn.tx.close()
        if result_list[1] == -1:
            try:
                chan_unicorn = yield unicorn.create(node,"")
                result = yield chan_unicorn.rx.get(timeout=timeout)
                yield chan_unicorn.tx.close()
            except:
                print "service unicorn fail while create main node"
                exit(1)
    except ServiceError as err:
        pass
    except:
        print "service unicorn fail while create node"
        exit(1)
    #set downtime if app name define
    if namespace.app:
        node_app = node + "/" + namespace.app
        value = str(int(time.time())) + "_" + str(namespace.time)
        try:
            chan_unicorn = yield unicorn.create(node_app,value)
            result = yield chan_unicorn.rx.get(timeout=timeout)
            yield chan_unicorn.tx.close()
            print "add downtime for app " + namespace.app
        except:
            try:
                chan_unicorn = yield unicorn.remove(node_app,0)
                result = yield chan_unicorn.rx.get(timeout=timeout)
                yield chan_unicorn.tx.close()
                chan_unicorn = yield unicorn.create(node_app,value)
                result = yield chan_unicorn.rx.get(timeout=timeout)
                yield chan_unicorn.tx.close()
                print "add downtime for app " + namespace.app
            except:
                print "service unicorn fail while add downtime"
                exit(1)
        exit(0)
    #remove downtime
    elif namespace.d:
        node_app = node + "/" + namespace.d
        try:
            chan_unicorn = yield unicorn.remove(node_app,0)
            result = yield chan_unicorn.rx.get(timeout=timeout)
            yield chan_unicorn.tx.close()
            print "remove downtime for app " + namespace.d
        except:
            print "no downtime for this app"
        exit(0)
    #list and removeall
    else:
        try:
            chan_unicorn = yield unicorn.children_subscribe(node)
            result = yield chan_unicorn.rx.get(timeout=timeout)
            list_children = result[1]
            yield chan_unicorn.tx.close()
        except:
            print "service unicorn fail while subscibe node"
            exit(1)
        if not list_children:
            print "downtime list is empty"
            exit(0)
        if namespace.list:
            t_now = int(time.time())
            print "downtime  app"
            for children in list_children:
                node_app = node + "/" + children
                try:
                    chan_unicorn = yield unicorn.get(node_app)
                    result = yield chan_unicorn.rx.get(timeout=timeout)
                    yield chan_unicorn.tx.close()
                except:
                    print "service unicorn fail while get list"
                    exit(1)
                t_last = int(result[0].split("_")[0])
                t_downtime = int(result[0].split("_")[1])*3600
                time_left = float(t_last + t_downtime - t_now)/3600
                if time_left < 0:
                    print "expired  " + children
                else:
                    print str(round(time_left,1)) + " hour  " + children
        elif namespace.rmall:
            for children in list_children:
                node_app = node + "/" + children
                try:
                    chan_unicorn = yield unicorn.remove(node_app,0)
                    result = yield chan_unicorn.rx.get(timeout=timeout)
                    yield chan_unicorn.tx.close()
                    print "remove downtime for " + children
                except:
                    print "service unicorn fail while removeall downtime"
                    exit(1)

parser = argparse.ArgumentParser()
parser.add_argument("-n","--app", metavar="name", type=str, help="app name to downtime")
parser.add_argument("-t","--time", type=int, default=1, help="time to downtime in hour(default 1 hour)")
parser.add_argument("-d", metavar="name", type=str, help="name app to delete downtime")
parser.add_argument("-l","--list", help="print all downtime", action="store_true")
parser.add_argument("--rmall", help="remove all downtime", action="store_true")
namespace = parser.parse_args()
if (namespace.list and namespace.rmall) or (namespace.app and namespace.d) or (not namespace.list and not namespace.rmall and not namespace.app and not namespace.d):
    parser.print_help()
    exit(0)
if namespace.time > int(336):
    print "very long time for downtime"
    exit(0)

try:
    pidor = ioloop.IOLoop.current().run_sync(main, timeout=30)
except ioloop.TimeoutError:
    print "2;timeout"
    exit(1)

