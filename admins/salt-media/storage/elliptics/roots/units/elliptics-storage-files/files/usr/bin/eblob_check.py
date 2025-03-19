#!/usr/bin/python

import os
import Queue
import re
import subprocess as s
import sys
from threading import Thread

class Worker(Thread):
    def __init__(self, out, queue):
        self.__queue = queue
        self.d = out
        Thread.__init__(self)

    def run(self):
        while 1:
	 if not self.__queue.empty():
            node = self.__queue.get()
	    parse(self.d,node)
	 else: break

def parse(_L,subdir):
	cmd = "eblob_scarab /srv/storage/%s/data-0*index | grep UNKNOWN | wc -l"%subdir
	p = s.Popen(cmd, stdout=s.PIPE, shell=True);
	res = p.communicate()
	_L[subdir] = res[0].strip("\n")

if __name__ == '__main__':
	L = {} 
	threads = []
	thread_count=8
	nodes=Queue.Queue(0)
	
	for i in os.listdir("/srv/storage/"):
            if re.match("\d+", i):
                for j in os.listdir("/srv/storage/{0}".format(i)):
                    if re.match("\d", j):
                        nodes.put("{0}/{1}".format(i,j))

	try:
		for i in range(thread_count):
			threads.append(Worker(L,nodes))
			threads[i].start()
	except Exception, e:
		print e

	for i in range(thread_count):
		threads[i].join()

	out = err_out = "2;Mastermind key affected on nodes "
	for i in L:
		if int(L[i]) > 1: out+="%s "%i 
	out="0;OK" if out == err_out else out
	print out

