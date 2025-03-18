#!/usr/bin/env python

import sys, os

c = 10000

def after(c, a):
    return c[c.find(a) + len(a):]

def before(c, a):
    return c[:c.find(a) + len(a) - 1]

def parsePort(c):
    c = after(c, "${")
    c = before(c, "}")
    c = after(c, "or ")

    return int(c)

def genNext(cgi):
    global c
    c += 1

    port = parsePort(cgi)

    cgi = after(cgi, "CgiSearchPrefix")
    cgi = cgi[1:]
    cgi = after(cgi, "http://")
    cgi = before(cgi, ":")

    return (c, cgi, port)

cp = ""

with open(sys.argv[1], 'r') as f:
    for i in f.readlines():
        i = i[:-1]
        p = i.find("CgiSearchPrefix")

        if p == -1:
            print i
        else:
            (port, host, rport) = genNext(i)

            print "        CgiSearchPrefix http://localhost:" + str(port) + "/yandsearch"
            cp += str(port) + ":" + str(host) + ":" + str(rport) + " "

sys.stdout.flush()

os.system("./cproxy " + cp)
