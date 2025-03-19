#!/usr/bin/env python
import requests
import time
import json
from tornado import gen
from tornado import httpclient
from tornado import ioloop
from cocaine.services import Service
import socket

@gen.coroutine
def push_check(unicorn, check_node, res, timeout):
    try:
        chan = yield unicorn.get(check_node)
        result = yield chan.rx.get(timeout=timeout)
        if str(result[0]) != str(res):
            chan = yield unicorn.put(check_node, str(res), result[1])
            result = yield chan.rx.get(timeout=timeout)
        yield chan.tx.close()
    except:
        chan = yield unicorn.create(check_node, str(res))
        result = yield chan.rx.get(timeout=timeout)
        yield chan.tx.close()

@gen.coroutine
def get_check(unicorn, check_node, timeout):
    try:
        chan = yield unicorn.get(check_node)
        result = yield chan.rx.get(timeout=timeout)
        yield chan.tx.close()
        r = result[0]
    except:
        r = ""
    raise gen.Return(r)

@gen.coroutine
def get_back_list(unicorn, node_backend_list, timeout):
    try:
        chan_b = yield unicorn.get(node_backend_list)
        result = yield chan_b.rx.get(timeout=timeout)
        yield chan_b.tx.close()
    except:
        print "2;service unicorn fail while read list of backend"
        exit(1)
    backend_list = []
    for i in result[0]:
        backend_list.append(i[0])
    raise gen.Return(backend_list)

@gen.coroutine
def main():
    #this check made for one minutes start
    hostname = socket.gethostname()
    #timeout in seconds
    timeout = 10
    #time than check is red(in minutes)
    critical_time = 15
    #check time in minutes
    check_time = [10, 20, 30]
    #ttl check time in seconds
    ttl_time = 540
    curl = "http://c.yandex-team.ru/api-cached/hosts/" + hostname + "?format=json"

    try:
        r = requests.get(curl,timeout=timeout)
    except:
        print "2;problem with conductor"
        exit(1)
    dc = json.loads(r.text)[0]["root_datacenter"]
    node_dc = "/uptime_check/" + str(dc)
    check_node = "/uptime_check/" + "state_" + str(dc)
    node_backend_list = "/depth_check/backend_list_" + str(dc)
    unicorn = Service("unicorn")

    try:
        chan_unicorn = yield unicorn.lock(node_dc)
        result = yield chan_unicorn.rx.get(timeout=timeout)
    except:
        print "0;lock on another front"
        yield chan_unicorn.tx.close()
        exit(0)
    yield gen.sleep(1)

    res = {}
    json_data = {}
    fail_back = 0
    backend_list = yield get_back_list(unicorn,node_backend_list,timeout)
    http = httpclient.AsyncHTTPClient()
    futures = {}

    for back in backend_list:
        curl = "http://" + back + ":3132/exec_pattern?pattern=uptime"
        futures[back] = gen.with_timeout(time.time() + timeout, http.fetch(curl))

    wait_iterator = gen.WaitIterator(**futures)

    while not wait_iterator.done():
        try:
            result = yield wait_iterator.next()
            uptime = int(result.body)
            res[wait_iterator.current_index] = uptime
        except:
            res[wait_iterator.current_index] = 0

    check = yield get_check(unicorn, check_node, timeout)
    try:
        check_json = json.loads(check)
    except:
        check_json = {}

    for back in backend_list:
        json_backend = {}
        json_backend["count"] = {}
        for t in check_time:
            if res[back] < (t*60):
                try:
                    json_backend["count"][str(t)] = int(check_json[back]["count"][str(t)]) + 1
                except:
                    json_backend["count"][str(t)] = 1
            else:
                json_backend["count"][str(t)] = 0
        #if red check ligth more then critical_time that flash check
        flash = False
        for i in json_backend["count"]:
            if json_backend["count"][str(i)] > (2*int(i) + 1 + critical_time):
                flash = True
            if flash:
                json_backend["count"][str(i)] = 0

        json_backend["uptime"] = res[back]
        json_backend["last_modify"] = int(time.time())

        json_data[back] = json_backend
    
    #save result of died node on ttl time
    for i in check_json:
        if i not in backend_list:
            if (int(time.time()) - int(check_json[i]["last_modify"])) < ttl_time:
                json_data[i] = check_json[i]

    result =  json.dumps(json_data, sort_keys=True, indent=4, separators=(',', ': '))
    yield push_check(unicorn, check_node, result, timeout)
    
    error = "2;"
    for back in json_data:
        for t in check_time:
            #detect flaping backend(2 restarts)
            if json_data[back]["count"][str(t)] > (2*int(t) + 1):
                error += back + ": flap with " + str(t) + " minutes uptime;" 

    if error != "2;":
        print error
    else:
        print "0;"

    yield chan_unicorn.tx.close()

try:
    pidor = ioloop.IOLoop.current().run_sync(main, timeout=35)
except ioloop.TimeoutError:
    print "2;timeout"
    exit(1)

