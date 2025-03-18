#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import datetime
import config
import threading

def createPath(path):
    if not os.path.exists(path):
        os.makedirs(path)

def removePath(path):
    if os.path.exists(path):
        os.remove(path)

def date2str(date):
    return date.strftime("%Y-%m-%d")

def str2date(s):
    return datetime.datetime.strptime(s, "%Y-%m-%d")

def date2timestamp(date):
    return int(date.strftime("%s"))

def getSerpName(date, name):
    return os.path.join(config.data_folder, date) + "/" + name + ".xml.gz"

def getWizName(date, name):
    return os.path.join(config.data_folder, date) + "/" + name + ".wiz.gz"

def getPointName(date, name):
    return os.path.join(config.data_folder, date) + "/" + name + ".point"

def getGraphName(date, streams):
    label = "_".join([s["file"] for s in streams])
    return  os.path.join(config.data_folder, date2str(date)) + "/graph_%s.xml" % label

def getTempName(date, name):
    return os.path.join(config.data_folder, date) + "/" + name + "_wiz.txt"

logLock = threading.Lock()
def log(text):
    try:
        logLock.acquire()
        with open(config.log, "a") as writer:
            t = "%s: %s\n" % (datetime.datetime.now().strftime("%d.%m.%Y - %H:%M:%S"), text)
            print t
            writer.write(t)
            writer.flush()
    finally:
        logLock.release()

createPath(config.data_folder)
