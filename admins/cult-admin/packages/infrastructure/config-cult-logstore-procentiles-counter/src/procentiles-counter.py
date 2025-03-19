#!/usr/bin/python
import multiprocessing
import sys, time
import os, fnmatch
import re
import yaml
import argparse
import numpy
import subprocess
import socket

import logging
logging.basicConfig(level=logging.DEBUG,
                    format='(%(threadName)-10s) %(message)s',
                    )
# Variables
## Argument Parser
argparser = argparse.ArgumentParser(description='Counts procentiles of times from logs')
argparser.add_argument('--config', action='store', default='/etc/procentiles-counter/procentiles-counter.cfg', help='Path to config file (default: /etc/procentiles-counter/procentiles-counter.cfg)')

# tool for read log
CMD="timetail -n %s %s | egrep %s \"%s\""
CMD_ATOMIC="timetail -n %s -b %s %s | egrep %s \"%s\""

# Functions
def find_files(directory, pattern):
    files_list = []
    simple_pattern = re.compile(pattern)
    for root, subFolders, files in os.walk(directory):
        for basefile in files:
            fullname = os.path.join(root, basefile)
            if simple_pattern.search(fullname):
                files_list.insert(-1,fullname)
    return files_list

def read_config(cfile):
        with open(cfile,'r') as f:
                config = yaml.load(f)
	return config

def get_log_list(group_list,log_root,log_name,hosts_regex):
        log_list = []
        if hosts_regex:
		compiled_regex = re.compile(hosts_regex)
		for filename in find_files(log_root, log_name):
                    if compiled_regex.search(filename):
                        log_list.insert(-1,filename)
        else:
            for group in group_list:
                    machines = os.listdir("%s/%s" % (log_root,group))
                    for machine in machines:
                            log_path = "/".join([log_root,group,machine,log_name])
                            log_list.insert(-1,log_path)
	return log_list

def operate_log(cmd,cmd_atomic,time_delta,log,fromtime,split_offset,grep_parameters,url):
        timings = []
        if (fromtime == 0):
            p = subprocess.Popen([cmd % (time_delta,log,grep_parameters,url)], shell=True, stdout=subprocess.PIPE)
        else:
            p = subprocess.Popen([cmd_atomic % (time_delta,fromtime,log,grep_parameters,url)], shell=True, stdout=subprocess.PIPE)
        for line in p.stdout.readlines():
#		time = int((round(float(line_regexp.search(line).group('REQUEST_TIME')) * 1000)))
                time = int(round(float(line.split('"')[split_offset].split(" ")[1]) * 1000))
                timings.insert(-1,time)
        return timings

def printer(project,group,name,times,percent,url):
	try:
		count = numpy.percentile(times,percent)
		# use sys.stdout for true unbuffered print data
		# 'print' causes some garbage data
		if url:
			sys.stdout.write(project + '.' + group  + '.' + url + '.' + name + str(percent) + ' ' + str(count) + '\n')
		else:
			sys.stdout.write(project + '.' + group  + '.' + name + str(percent) + ' ' + str(count) + '\n')
		sys.stdout.flush()
	except:
		# nothing here
		count = 0
	return

def worker(agroup, project, split_offset, url={"": ""}, exclude=False):
	TIMES = []
	GROUPS = []
	HOSTS_REGEX=""
	grep_parameters=""
	try:
		HOSTS_REGEX = config["PROJECTS"][project][agroup]["HOST_REGEX"]
	except:
		GROUPS = config["PROJECTS"][project][agroup]["GROUPS"]

	if exclude == True: grep_parameters = "-v"

	for log in get_log_list(GROUPS,LOG_ROOT,LOG_NAME, HOSTS_REGEX):
		TIMES = TIMES + operate_log(CMD,CMD_ATOMIC,TIME_DELTA,log,fromtime, split_offset, grep_parameters, url.values()[0])
	PERCENTILES = config["PROJECTS"][project][agroup]["PERCENTS"]
	for percent in PERCENTILES:
		printer(project,agroup,"reqtime",TIMES,percent,url.keys()[0])
	return
	
# Fire!
if __name__ == "__main__":
	# Read config
	config = read_config(argparser.parse_args().config)
	# Bind variables

	TIME_DELTA = config["TIME_DELTA"]

        # workers settings and count
        try:
            USE_MULTIPROCESSING = config["USE_MULTIPROCESSING"]
        except:
            USE_MULTIPROCESSING = 0

        try:
            WORKER_COUNT = config["WORKER_COUNT"]
	    if WORKER_COUNT < 1:
               WORKER_COUNT = 1
        except:
            WORKER_COUNT = 2

        # ATOMIC setting (all log in one time)
	try:
            ATOMIC = config["ATOMIC"]
        except:
            ATOMIC = 0
	
        if (ATOMIC == 1):
             fromtime = int(time.time())

             # TIME_DELAY setting (get time from logs with delay) 
             try:
                 TIME_DELAY = config["TIME_DELAY"]
             except:
                 TIME_DELAY = 0

             if (TIME_DELAY > 0):
                  fromtime -= TIME_DELAY
        else:
             fromtime = 0

        try:
            SPLIT_OFFSET = config["SPLIT_OFFSET"]
        except:
            SPLIT_OFFSET = -9

	LOG_ROOT = config["LOG_ROOT"] + '/' + socket.gethostname()
	LOG_NAME = config["LOG_NAME"]
#       LINE_REGEXP = re.compile(config["LINE_REGEXP"])
	PROJECTS = config["PROJECTS"].keys()
	# Get times

	for project in PROJECTS:
		# Bind groups for aggregate
		AGROUPS = config["PROJECTS"][project].keys()
		if USE_MULTIPROCESSING:
	                jobs = []
	                while True:
	                        if len(AGROUPS) == 0:
	                                break
	                        if len(AGROUPS) < WORKER_COUNT:
	                                for i in range(len(AGROUPS)):
	                                        curr = AGROUPS.pop()
	                                        process = multiprocessing.Process(target=worker,args=(curr,project,SPLIT_OFFSET))
	                                        jobs.append(process)
	                        if len(AGROUPS) >= WORKER_COUNT:
	                                for i in range(WORKER_COUNT):
	                                        curr = AGROUPS.pop()
	                                        process = multiprocessing.Process(target=worker,args=(curr,project,SPLIT_OFFSET))
	                                        jobs.append(process)
	                        for j in jobs:
	                                j.start()
	
	                        for j in jobs:
	                                j.join()
	
	                        del jobs[:]
		else:
			for agroup in AGROUPS:
				try:
					URLS = config["PROJECTS"][project][agroup]["URLS"]
					for url in URLS:
						worker(agroup,project,SPLIT_OFFSET,url)
				except:
					pass
				try:
					EXCLUDE_URLS = config["PROJECTS"][project][agroup]["EXCLUDE_URLS"]
					for url in EXCLUDE_URLS:
						worker(agroup,project,SPLIT_OFFSET,url, exclude = True)
				except:
					pass
	
				worker(agroup,project,SPLIT_OFFSET)
	sys.exit(0)
