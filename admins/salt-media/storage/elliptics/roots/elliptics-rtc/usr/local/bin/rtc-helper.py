#!/usr/bin/python

import os,sys,json,re,copy
import socket

if sys.argv[1] == "gen-ell-conf":
    if not os.path.isdir("/srv/storage/"):
        print("ERROR: /srv/storage/ not exist.")
    backend_ids = []
    datamap = [
#        {
#            "disk": "-1",
#            "ddir": "-1",
#            "backend_id": "-1",
#            "group": "-1"
#        }
    ]

    numcheck = re.compile("^[0-9]*$")
    diskdirs = os.listdir("/srv/storage/")
    bid = "" # str now
    for d in diskdirs:
        if numcheck.match(d):
            backdirs = os.listdir("/srv/storage/%s/" % d)
            for b in backdirs:
                if numcheck.match(b):
                    with open("/srv/storage/%s/%s/kdb/group.id" % (d,b)) as f:
                        gid = str(f.read())
                    if numcheck.match(gid):
                        bid = "9{0}0{1}".format(d,b)
                        _back = {
                            "disk": d,
                            "ddir": b,
                            "backend_id": int(bid),
                            "group": int(gid)
                        }
                        datamap.append(_back)
    fqdn = socket.getfqdn()
    config = {
       "backends" : [
       ],
       "options" : {
          "server_net_prio" : 1,
          "monitor" : {
             "port" : 10025,
             "top" : {
                "period_in_seconds" : 120,
                "events_size" : 1000000,
                "top_length" : 50
             },
             "call_tree_timeout" : 10,
             "history_length" : 20000
          },
          "indexes_shard_count" : 16,
          "client_net_prio" : 6,
          "parallel" : True,
          "flags" : 4,
          "remote" : [
             "s03myt.mdst.yandex.net:1025:10",
             "s01iva.mdst.yandex.net:1025:10",
             "s02iva.mdst.yandex.net:1025:10",
             "s01man.mdst.yandex.net:1025:10",
             "s01sas.mdst.yandex.net:1025:10",
             "s01vla.mdst.yandex.net:1025:10",
             "s04sas.mdst.yandex.net:1025:10"
          ],
          "wait_timeout" : 3,
          "auth_cookie" : "media_test_elliptics_storage",
          "daemon" : False,
          "send_limit" : 100,
          "address" : [
              "{0}:1025:10-0".format(fqdn)
          ],
          "handystats_config" : "/etc/elliptics/handystats.json",
          "queue_timeout" : "3",
          "net_thread_num" : 10,
          "io_thread_num" : 16,
          "join" : True,
          "tls" : {
             "support" : 1,
             "ca_path" : "/etc/elliptics/ssl/",
             "key_path" : "/etc/elliptics/ssl/storage.key",
             "cert_path" : "/etc/elliptics/ssl/storage.crt"
          },
          "nonblocking_io_thread_num" : 10,
          "check_timeout" : 100
       },
       "logger" : {
          "level" : "error",
          "access" : [
             {
                "sinks" : [
                   {
                      "factor" : 20,
                      "sink" : {
                         "type" : "file",
                         "path" : "/dev/stdout",
                         "flush" : 1
                      },
                      "overflow" : "wait",
                      "type" : "asynchronous"
                   }
                ],
                "formatter" : {
                   "type" : "tskv",
                   "create" : {
                      "type" : "elliptics-storage",
                      "tskv_format" : "elliptics-storage-server-log"
                   },
                   "remove" : [
                      "severity",
                      "message"
                   ],
                   "mutate" : {
                      "timestamp" : {
                         "gmtime" : False,
                         "strftime" : "%Y-%m-%dT%H:%M:%S"
                      },
                      "unixtime_microsec_utc" : {
                         "gmtime" : False,
                         "strftime" : "%s.%f"
                      },
                      "timezone" : {
                         "strftime" : "%z"
                      }
                   }
                }
             }
          ],
          "core" : [
             {
                "sinks" : [
                   {
                      "sink" : {
                         "type" : "file",
                         "flush" : 1,
                         "path" : "/var/log/elliptics/node-1.log"
                      },
                      "factor" : 20,
                      "type" : "asynchronous",
                      "overflow" : "wait"
                   }
                ],
                "formatter" : {
                   "sevmap" : [
                      "DEBUG",
                      "NOTICE",
                      "INFO",
                      "WARNING",
                      "ERROR"
                   ],
                   "type" : "string",
                   "pattern" : "{timestamp:l} {trace_id:{0:default}0>16}/{thread:d}/{process} {severity}: {message}, attrs: [{...}]"
                }
             }
          ]
       }
    }

    ell_backend = {
        "queue_timeout" : "3",
        "group" : "NULL",
        "datasort_dir" : "/cache/defrag/",
        "index_block_bloom_length" : "5120",
        "defrag_timeout" : "3600",
        "bg_ioprio_class" : 3,
        "sync" : "4",
        "blob_size_limit" : "916G",
        "blob_size" : "50G",
        "defrag_percentage" : "5",
        "history" : "NULL",
        "enable" : True,
        "pool_id" : "NULL",
        "records_in_blob" : "1000000",
        "data" : "NULL",
        "bg_ioprio_data" : 0,
        "backend_id" : "NULL",
        "blob_flags" : "2112",
        "type" : "blob"
    }

    for b in datamap:
        _ell_backend = copy.deepcopy(ell_backend)
        _ell_backend["group"] = b["group"]
        _ell_backend["backend_id"] = b["backend_id"]
        _ell_backend["pool_id"] = b["disk"]
        _ell_backend["history"] = "/srv/storage/%s/%s/kdb" % (b["disk"], b["ddir"])
        _ell_backend["data"] = "/srv/storage/%s/%s/data" % (b["disk"], b["ddir"])
        config["backends"].append(_ell_backend)   

    with open('/etc/elliptics/elliptics-rtc.conf', 'w') as f:
        json.dump(config, f)


