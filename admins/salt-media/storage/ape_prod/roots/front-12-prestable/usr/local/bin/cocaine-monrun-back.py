#!/usr/bin/env python
import requests
import argparse
import time
import json
from tornado import gen
from tornado import httpclient
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
    timeout = 30
    fail_percent = 40
    curl = "http://c.yandex-team.ru/api-cached/hosts/" + hostname + "?format=json"

    try:
        r = requests.get(curl,timeout=5)
    except:
        print "2;problem with conductor"
        exit(1)
    dc = json.loads(r.text)[0]["root_datacenter"]
    node_dc = "/monrun_check/" + str(dc) + "_" + str(namespace.n)
    node_backend_list = "/depth_check/backend_list_" + str(dc)
    unicorn = Service("unicorn")

    try:
        chan_unicorn = yield unicorn.lock(node_dc)
        result = yield chan_unicorn.rx.get(timeout=2)
    except:
        print "0;lock on another front"
        yield chan_unicorn.tx.close()
        exit(0)
    yield gen.sleep(1)

    backend_list = yield get_back_list(unicorn,node_backend_list,timeout)
    error = {}
    warning = {}
    failed_list = ""
    ok = "0;"
    fail_back = 0
    number_of_back = len(backend_list)
    http = httpclient.AsyncHTTPClient()
    futures = {}

    for back in backend_list:
        curl = "http://" + back + ":3132/exec_pattern?pattern=monrun&stdin=" + namespace.n
        futures[back] = gen.with_timeout(time.time() + timeout, http.fetch(curl))

    wait_iterator = gen.WaitIterator(**futures)

    while not wait_iterator.done():
        try:
            result = yield wait_iterator.next()
            code = int(result.body.split(";")[1][0])
            if code == 2:
                error[wait_iterator.current_index] = result.body.split(";")[2:][0]
                error[wait_iterator.current_index] = str(error[wait_iterator.current_index]).split("\n")[0]
            elif code == 1:
                warning[wait_iterator.current_index] = result.body.split(";")[2:][0]
                warning[wait_iterator.current_index] = str(warning[wait_iterator.current_index]).split("\n")[0]
        except:
            fail_back = fail_back + 1
            if failed_list == "":
                failed_list = wait_iterator.current_index
            else:
                failed_list = failed_list + "," + wait_iterator.current_index

    percent =  int(float(fail_back)/float(number_of_back)*100)

    if percent > fail_percent:
        print "2;" + str(percent) + "% of backends timeout or failed(" + failed_list + ")"
        yield chan_unicorn.tx.close()
        exit(1)
    if error == {} and warning == {}:
        if percent == 0:
            print "0;Ok"
        else:
            print "1;" + str(percent) + "% of backends timeout or failed(" + failed_list + ")"
    elif error != {}:
        #uniq error values
        error_count = len(error)
        error_uniq_count = len(set(error.values()))
        error_message = "2;"
        if error_count > 1 and error_uniq_count > 1:
            for i in error.keys():
                error_message = error_message + i + ":" + error[i] + ","
        elif error_count > 1 and error_uniq_count == 1:
            error_message = error_message + str(error_count) + " backends fail:" + error[error.keys()[0]]
        elif error_count == 1 and error_uniq_count == 1:
            key = error.keys()[0]
            error_message = error_message + key + ":" + error[key]
        print error_message
        yield chan_unicorn.tx.close()
        exit(1)
    elif warning != {}:
        #uniq warning values
        warning_count = len(warning)
        warning_uniq_count = len(set(warning.values()))
        warning_message = "1;"
        if warning_count > 1 and warning_uniq_count > 1:
            for i in warning.keys():
                warning_message = warning_message + i + ":" + warning[i]
        elif warning_count > 1 and warning_uniq_count == 1:
            warning_message = warning_message + str(warning_count) + " backends warning:" + warning[warning.keys()[0]]
        elif warning_count == 1 and warning_uniq_count == 1:
            key = warning.keys()[0]
            warning_message = warning_message + key + ":" + warning[key]
        print warning_message
        yield chan_unicorn.tx.close()
        exit(0)

    yield chan_unicorn.tx.close()

parser = argparse.ArgumentParser()
parser.add_argument("-n", metavar="name", type=str, help="name of monrun check")
namespace = parser.parse_args()
if not namespace.n:
    parser.print_help()
    exit(0)

try:
    pidor = ioloop.IOLoop.current().run_sync(main, timeout=50)
except ioloop.TimeoutError:
    print "2;timeout"
    exit(1)
