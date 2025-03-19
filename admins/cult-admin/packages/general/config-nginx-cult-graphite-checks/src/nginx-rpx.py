#!/usr/bin/python
import os
import yaml
# Variables
CHECKS_DIR = "/etc/nginx-cult-graphite-confs"
CHECKS = os.listdir(CHECKS_DIR)

# Functions
def read_config(cfile):
	with open(cfile,'r') as f:
		return yaml.load(f)

def read_log(log):
	with open(log,'r') as f:
        	return f.readlines()

def operate_log(lines):
	for line in lines:
		result = line.rstrip('\n').split(" ")
		if result[CODE_POS].isdigit():
			for host in HOSTS.keys():
				if host == "all" or result[HOST_POS] in HOSTS[host]["hosts"]:
					HOSTS[host]["rpm"][result[CODE_POS]] = HOSTS[host]["rpm"].setdefault(result[CODE_POS], 0) + 1

def printer(host,rtype,time_delta):
	for code in HOSTS[host]["rpm"].keys():
		if rtype == 'rps':
			HOSTS[host]["rps"][code] = float(HOSTS[host]["rpm"][code]) / time_delta
		print("%s.%s %s" % (host,code,HOSTS[host][rtype][code]))

# Fire!
if __name__ == "__main__":
	# Read config
	for check in CHECKS:
		check_path = "/".join([CHECKS_DIR,check])
		config = read_config(check_path)
		# Bind variables
		HOSTS = config["HOSTS"]
		TIME_DELTA = config["TIME_DELTA"]
		LOG = config["LOG"]
		HOST_POS = config["HOST_POSITION"]
		CODE_POS = config["CODE_POSITION"]

		# Create counters
		for host in HOSTS.keys():
			HOSTS[host]["rpm"] = {}
			if HOSTS[host]["type"] == 'rps':
				HOSTS[host]["rps"] = {}

		# Make operations
		operate_log(read_log(LOG))

		# Print result
		for host in HOSTS.keys():
			printer(host,HOSTS[host]["type"],TIME_DELTA)
