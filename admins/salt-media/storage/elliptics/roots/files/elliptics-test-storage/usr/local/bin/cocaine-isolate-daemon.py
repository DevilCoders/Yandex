#!/usr/bin/env python
import requests
import json

curl = "http://127.0.0.1:8000/metrics"
try:
    r = requests.get(curl)
except:
    print "2;problem with connect to daemon"
    exit(1)
try:
    daemon_connections = json.loads(r.text)["daemon_connections"]["count"]
    daemon_goroutines = json.loads(r.text)["daemon_goroutines"]["value"]
    daemon_hc_openfd = json.loads(r.text)["daemon_hc_openfd"]["error"]
    daemon_hc_threads = json.loads(r.text)["daemon_hc_threads"]["error"]
    process_spawning_queue_size = json.loads(r.text)["process_spawning_queue_size"]["count"]
except:
    print "2;problem with parse json or 404"
    exit(1)
if int(process_spawning_queue_size) > 15:
    print "2;process_spawning_queue_size more then 15"
    exit(1)
if int(daemon_connections) > 3000:
    print "2;daemon_connections more then 3000"
    exit(1)
if int(daemon_goroutines) > 10000:
    print "1;daemon_goroutines more then 10000"
    exit(1)
if daemon_hc_openfd:
    print "2;daemon_hc_openfd " + str(daemon_hc_openfd)
    exit(1)
if daemon_hc_threads:
    print "2;daemon_hc_threads " + str(daemon_hc_threads)
    exit(1)
print "0;Ok"

