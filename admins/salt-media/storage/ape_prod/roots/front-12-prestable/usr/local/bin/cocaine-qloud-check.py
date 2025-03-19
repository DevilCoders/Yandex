#!/usr/bin/env python
import requests
import argparse
import json
from tornado import gen
from tornado import ioloop
from cocaine.services import Service
import socket

@gen.coroutine
def get_back_list(unicorn,node_backend_list,timeout):
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
    hostname = socket.gethostname()
    curl_timeout = 5
    timeout = 3
    part_of_qloud_fqdn = "qloud-c"
    fail_percent = 10
    fail_backend_number = 3
    if namespace.percent:
        fail_percent = namespace.percent
    if namespace.minimal:
        fail_backend_number = namespace.minimal
    curl = "http://c.yandex-team.ru/api-cached/hosts/" + hostname + "?format=json"
    try:
        r = requests.get(curl,timeout=curl_timeout)
    except:
        print "2;problem with conductor"
        exit(1)
    dc = json.loads(r.text)[0]["root_datacenter"]
    #node_dc node where get lock
    node_dc = "/monrun_check/qloud_lock_" + str(dc)
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
    list_from_unicorn = yield get_back_list(unicorn,node_backend_list,timeout)
    qloud_list_unicorn = []
    for i in list_from_unicorn:
        if part_of_qloud_fqdn in i:
            qloud_list_unicorn.append(i)
    number_qloud_unicorn = len(qloud_list_unicorn)
    curl = namespace.url
    try:
        r = requests.get(curl,timeout=curl_timeout)
    except:
        try:
            r = requests.get(curl,timeout=curl_timeout)
        except:
            print "2;problem with " + namespace.url
            exit(1)
    list_url = r.text.split("\n")
    qloud_list_unicorn = []
    for i in list_url:
        if dc == i.split("\t")[0]:
            qloud_list_unicorn.append(i.split("\t")[1])
    number_qloud_url = len(qloud_list_unicorn)
    try:
        percent = int(100) - int(float(number_qloud_unicorn)/float(number_qloud_url)*100)
    except:
        percent = 0
    if number_qloud_unicorn < fail_backend_number:
        print "2;number of backens less then minimal"
    elif number_qloud_unicorn == 0 or number_qloud_url == 0:
        print "2;one of backend number is null"
    elif number_qloud_unicorn == number_qloud_url:
        print "0;Ok"
    elif percent > fail_percent:
        print "2;" + str(percent) + "% fail"
    else:
        print "1;" + str(percent) + "% fail"
    yield chan_unicorn.tx.close()

parser = argparse.ArgumentParser()
parser.add_argument("-u", "--url", type=str, help="url return backends(example:http://front.intape.yandex.net/cq-list__v012/http/?qloud=cocaine.cocaine-test-api.test)")
parser.add_argument("-m", "--minimal", type=int, help="number of minimal backends in this dc(3 by default)")
parser.add_argument("-p", "--percent", type=int, help="critical percent in this dc(default 10%)")
namespace = parser.parse_args()
if not namespace.url:
    parser.print_help()
    exit(0)

try:
    pidor = ioloop.IOLoop.current().run_sync(main, timeout=15)
except ioloop.TimeoutError:
    print "2;timeout"
    exit(1)

