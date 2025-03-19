#!/usr/bin/env python
import fnmatch
import requests
import time
import json
import fnmatch
from tornado import gen
from tornado import ioloop
from cocaine.services import Service
import socket

@gen.coroutine
def put_backend_list(unicorn,node_backend_list,list_backend,timeout):
    #put backen list of this dc
    try:
        chan_b = yield unicorn.create(node_backend_list,list_backend)
        result = yield chan_b.rx.get(timeout=timeout)
        yield chan_unicorn.tx.close()
    except:
        try:
            chan_b = yield unicorn.get(node_backend_list)
            result = yield chan_b.rx.get(timeout=timeout)
            yield chan_b.tx.close()
            if str(result[0]) != str(list_backend):
                chan_b = yield unicorn.put(node_backend_list,list_backend,result[1])
                result = yield chan_b.rx.get(timeout=timeout)
                yield chan_b.tx.close()
        except:
            print "2;unicorn fail while write backend list"
            exit(1)

@gen.coroutine
def node_check(backend, timeout, list_downtime):
    result = {}
    try:
        node_back = Service("node", endpoints=[[backend[0],int(backend[1])]])
        chan = yield node_back.list()
        app_list = yield chan.rx.get(timeout=timeout)
        for name in app_list:
            match = 0
            for down in list_downtime:
                if fnmatch.fnmatchcase(name, down):
                    match = 1
            if match == 0:
                try:
                    chan = yield node_back.info(name,0x1)
                    info = yield chan.rx.get(timeout=timeout)
                    if info["state"] != "running":
                        if "warning" in result:
                            result["warning"] = result["warning"] + "," + name
                        else:
                            result["warning"] = name
                    elif info["queue"]["depth"] == info["queue"]["capacity"]:
                        if "error" in result:
                            result["error"] = result["error"] + "," + name
                        else:
                            result["error"] = name
                except:
                    #check error code number
                    try:
                        chan = yield node_back.info(name)
                        info = yield chan.rx.get(timeout=timeout)
                        if info["error"] != 14:
                            if "warning" in result:
                                result["warning"] = result["warning"] + "," + name
                            else:
                                result["warning"] = name
                    except:
                        if "warning" in result:
                            result["warning"] = result["warning"] + "," + name
                        else:
                            result["warning"] = name
    except:
        result["fail"] = int(1)
    raise gen.Return(result)

@gen.coroutine
def main():
    hostname = socket.gethostname()
    curl = "http://c.yandex-team.ru/api-cached/hosts/" + hostname + "?format=json"
    try:
        r = requests.get(curl)
    except:
        print "2;problem with conductor"
        exit(1)
    dc = json.loads(r.text)[0]["root_datacenter"]
    node_dc = "/depth_check/" + str(dc)
    node_backend_list = "/depth_check/backend_list_" + str(dc)
    node_downtime = "/depth_check/downtime"
    unicorn = Service("unicorn")
    try:
        chan_unicorn = yield unicorn.lock(node_dc)
        result = yield chan_unicorn.rx.get(timeout=1)
    except:
        try:
            print "0;lock on another front"
        except:
            print "2;service unicorn fail"
            yield chan_unicorn.tx.close()
            exit(1)
        yield chan_unicorn.tx.close()
        exit(0)
    yield gen.sleep(1)
    #ip6 = socket.getaddrinfo(hostname, None, socket.AF_INET6)[0][4][0]
    list_backend = []
    list_endpoint_locator_fail = ""
    list_endpoint_node_fail= ""
    timeout = 3
    locator = Service("locator")
    try:
        chan = yield locator.cluster()
    except:
        print "2;error while connect to local service locator"
        yield chan_unicorn.tx.close()
        exit(1)
    cluster_local = yield chan.rx.get(timeout=timeout)
    if not cluster_local:
        print "2;local cluster list is null"
        yield chan_unicorn.tx.close()
        exit(1)
    for tushka in cluster_local:
        try:
            hostname_tushka = socket.getfqdn(cluster_local[tushka][0])
            if hostname_tushka != hostname:
                #remove storage host from cocaine discovery
                if "-kvm" not in hostname_tushka and "storage" not in hostname_tushka:
                    if "front" not in hostname_tushka:
                        if hostname_tushka == "cocs09e.ape.yandex.net" or hostname_tushka == "cocs10e.ape.yandex.net" or hostname_tushka == "cocs12e.ape.yandex.net" or str(dc) in hostname_tushka:
                            list_backend.append([hostname_tushka,int(cluster_local[tushka][1])])
                #remove check front of backend by locator
                #locator_tushka = Service("locator", endpoints=[[cluster_local[tushka][0],int(cluster_local[tushka][1])]])
                #cluster_remote = yield (yield gen.with_timeout(time.time() + timeout, locator_tushka.cluster())).rx.get(timeout=timeout)
                #if not cluster_remote:
                #    list_backend.append([hostname_tushka,int(cluster_local[tushka][1])])
        except:
            if list_endpoint_locator_fail == "":
                list_endpoint_locator_fail = "2;remote backend with fail locator: " + (str(socket.getfqdn(cluster_local[tushka][0])))
            else:
                list_endpoint_locator_fail = list_endpoint_locator_fail + "," + (str(socket.getfqdn(cluster_local[tushka][0])))
    if not list_backend:
        print "2;no backend in local cluster list"
        yield chan_unicorn.tx.close()
        exit(1)
    #write list_backend to unicorn
    put_backend_list(unicorn,node_backend_list,list_backend,timeout)
    #read downtime list from unicorn
    try:
        chan_unicorn_downtime = yield unicorn.children_subscribe(node_downtime)
        result = yield chan_unicorn_downtime.rx.get(timeout=timeout)
        list_children = result[1]
        yield chan_unicorn_downtime.tx.close()
    except:
        list_children = []
    list_downtime = []
    if list_children != []:
        #remove expired downtime and create downtime list
        for children in list_children:
            t_now = int(time.time())
            node_app = node_downtime + "/" + children
            try:
                chan_unicorn_downtime = yield unicorn.get(node_app)
                result = yield chan_unicorn_downtime.rx.get(timeout=timeout)
                yield chan_unicorn_downtime.tx.close()
                t_last = int(result[0].split("_")[0])
                t_downtime = int(result[0].split("_")[1])*3600
                time_left = float(t_last + t_downtime - t_now)/3600
                if time_left < 0:
                    try:
                        chan_unicorn_downtime = yield unicorn.remove(node_app,0)
                        result = yield chan_unicorn_downtime.rx.get(timeout=timeout)
                        yield chan_unicorn_downtime.tx.close()
                    except:
                        #downtime remove on another node
                        pass
                else:
                    list_downtime.append(children)
            except:
                #downtime remove on another node
                pass

    warning = "app status broken: "
    error = "depth is full: "
    futures = {}
    result = {}
    for backend in list_backend:
        futures[backend[0]] = node_check(backend, timeout, list_downtime)
    wait_iterator = gen.WaitIterator(**futures)
    while not wait_iterator.done():
        try:
            result = yield wait_iterator.next()
            if "error" in result:
                error = error + str(wait_iterator.current_index) + ": " + result["error"] + "; "
            elif "warning" in result:
                warning = warning + str(wait_iterator.current_index) + ": " + result["warning"] + "; "
            elif "fail" in result:
                list_endpoint_node_fail = list_endpoint_node_fail + (str(wait_iterator.current_index)) + ","
        except:
            list_endpoint_node_fail = list_endpoint_node_fail + (str(wait_iterator.current_index)) + ","

    list_fail = ""
    if list_endpoint_locator_fail == "" and list_endpoint_node_fail != "":
        list_fail = "2;remote backend with fail node: " + str(list_endpoint_node_fail)
    elif list_endpoint_locator_fail != "" and list_endpoint_node_fail == "":
        list_fail = list_endpoint_locator_fail
    elif list_endpoint_locator_fail != "" and list_endpoint_node_fail != "":
        list_fail = str(list_endpoint_locator_fail) + " remote backend with fail node: " + str(list_endpoint_node_fail)
    if list_fail != "":
        if error != "depth is full: " and warning != "app status broken: ":
            print str(list_fail) + "; " + str(error) + "; " + str(warning)
            yield chan_unicorn.tx.close()
            exit(1)
        elif error != "depth is full: ":
            print str(list_fail) + "; " + str(error)
            yield chan_unicorn.tx.close()
            exit(1)
        elif warning != "app status broken: ":
            print str(list_fail) + "; " + str(warning)
            yield chan_unicorn.tx.close()
            exit(1)
        else:
            print str(list_fail)
            yield chan_unicorn.tx.close()
            exit(1)
    elif error != "depth is full: ":
        if warning != "app status broken: ":
            print "2;" + error + " and " + warning
            yield chan_unicorn.tx.close()
            exit(1)
        else:
            print "2;" + error
            yield chan_unicorn.tx.close()
            exit(1)
    elif warning != "app status broken: ":
        print "1;" + warning
        yield chan_unicorn.tx.close()
        exit(0)
    else:
        print "0;Ok"
        yield chan_unicorn.tx.close()
        exit(0)
    yield chan_unicorn.tx.close()

try:
    pidor = ioloop.IOLoop.current().run_sync(main, timeout=55)
except ioloop.TimeoutError:
    print "2;timeout"
    exit(1)
