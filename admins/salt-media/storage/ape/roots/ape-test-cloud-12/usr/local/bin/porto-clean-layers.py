#!/usr/bin/env python
import json
import os
import time

runlist = "default-12"
try:
    runlist_list = os.popen("cocaine-tool runlist view -n " + runlist).readlines()
    layer_portoctl = os.popen('portoctl layer -L | grep -v ISS-AGENT').readlines()
    #wait while isolation daemon sync journal to disk
    time.sleep(65)
    with open('/ephemeral/portojournal.jrnl') as data_file:    
        layer_daemon = json.load(data_file)["layers"]
    #make diff from three list
    for p in layer_portoctl:
        p_fix = p.split("\n")[0]
        if p_fix not in str(layer_daemon):
            print "remove layer(not find in portojournal.jrnl): " + p_fix
            rm = os.popen("portoctl layer -R " + p_fix)
        else:
            layer = p_fix[0:p_fix.rfind("_")]
            detect = 0
            for z in runlist_list:
                try:
                    app = z.split("\"")[1].replace(":","_")
                    if app == layer:
                        detect = 1
                except:
                    continue
            if detect == 0:
                print "remove layer(not find in runlist): " + p_fix
                rm = os.popen("portoctl layer -R " + p_fix)
except:
    print "Chet ne tak poshlo!"

